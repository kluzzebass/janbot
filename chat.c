/* 
 * JanBot, Concurrent file server for iRC.
 * Copyright (C) 1996,1997,1998 Jan Fredrik Leversund 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: chat.c,v 1.5 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>


/* The list of DCC CHAT connections */
struct chat_t *chatlist=NULL;


/* Function: Sets up a new chat connection for a user. */
int add_chat(char *nick,struct chat_t **clist)
{
	struct chat_t *tmp;
	
	if (!(tmp=(struct chat_t *)malloc(sizeof(struct chat_t))))
	{
		/* Could not allocate memory */
		DEBUG0("malloc(): Could not allocate memory.\n");
		return(1);
	}
	memset(tmp,0,sizeof(struct chat_t));

	/* Changed from get_user() to get_alias() to support aliases. */
	if (!(tmp->chatter=get_alias(nick,&users)))
	{
		/* No such user */
		free(tmp);
		DEBUG0("No such user.\n");
		return(2);
	}
	
	if (find_chat(tmp->chatter,clist))
	{
		/* User already has an open chat. */
		free(tmp);
		DEBUG0("Chat already open.\n");
		return(3);
	}


	/* Now we can set up a chat connection, spawn a child and so on... */

	if ( (tmp->oldsockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		/* Oops, can't create a socket. That sux. */
		free(tmp);
		DEBUG0("socket(): Unable to create socket.\n");

		return(4);
	}
	
	memset(&tmp->srv_addr,0,sizeof(tmp->srv_addr));
	tmp->srv_addr.sin_family=AF_INET;
	tmp->srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	tmp->port=next_port();
	tmp->srv_addr.sin_port=htons(tmp->port);
	
	while (bind(tmp->oldsockfd,(struct sockaddr *)&tmp->srv_addr,sizeof(tmp->srv_addr))<0)
	{
		if (errno!=EADDRINUSE)
		{
			/* Damnit, can't bind local address... */
			close(tmp->oldsockfd);
			DEBUG0("bind(): Cannot bind local address.\n");
			free(tmp);
			return(5);
		}
		else
		{
			tmp->port=next_port();
			tmp->srv_addr.sin_port=htons(tmp->port);
		}
	}
	
	if (listen(tmp->oldsockfd,4))
	{
		/* Rats, listen won't work... */
		close(tmp->oldsockfd);
		DEBUG0("listen() is broken..\n");

		free(tmp);
		return(6);
	}

	if (fcntl(tmp->oldsockfd,F_SETFL,O_NONBLOCK)<0)
	{
		/* With the sinking sensation, I'm in deep deep trouble... */
		close(tmp->oldsockfd);
		DEBUG0("fcntl() won't work.\n");

		free(tmp);
		return(7);
	}

	if (socketpair(AF_UNIX,SOCK_STREAM,0,tmp->ipc)==-1)
	{
		/* Okay, No point maintaining this conversation. */
		close(tmp->oldsockfd);
		DEBUG0("socketpair() won't work.\n");
		free(tmp);
		return(8);
	}

	if (fcntl(tmp->ipc[0],F_SETFL,O_NONBLOCK)<0)
	{
		/* Ehm... */
		close(tmp->oldsockfd);
		close(tmp->ipc[0]);
		close(tmp->ipc[1]);
		DEBUG0("fcntl() on socketpair won't work.\n");
		free(tmp);
		return(9);
	}
	
	if ((tmp->childpid=fork())==-1)
	{
		close(tmp->oldsockfd);
		close(tmp->ipc[0]);
		close(tmp->ipc[1]);
		DEBUG0("fork() broke.\n");
		free(tmp);
		return(10);
	}

	if (tmp->childpid)
	{
		/* Parent process. */
		DEBUG0("Connection should be set up.\n");
		close(tmp->ipc[1]);
	}
	else
	{
		/* Child process. */
		init_child(tmp);
		while(process_child());
		_exit(0);
	}

	tmp->terminate=0; /* Do not mark for termination. */
	tmp->active=0; /* We're not connected yet. */
	tmp->filenum=0; /* I bet we're not sending any files yet. */
	tmp->queued=0; /* And we certainly have no files enqueued. */
	mystrncpy(tmp->nick,nick,NICK_LEN); /* Store the current nick. */
	
	/* Set both starttime and idletime... */
	tmp->stime=tmp->itime=time(NULL);

	/* No need to sort this list, just put the next one on top. */
	tmp->next=*clist;
	*clist=tmp;

	send_to_server("PRIVMSG %s :\001DCC CHAT chat %lu %u\001\n",tmp->nick,local_ip,tmp->port);

	return(0);
}

/* Function: Searches the chat list for a user. */
struct chat_t *find_chat(struct user_t *u,struct chat_t **clist)
{
	struct chat_t *tmp=*clist;
	while ((tmp)&&(tmp->chatter!=u))
	{
		tmp=tmp->next;
	}
	return(tmp);
}

/* Function: Removes a chat connection from the chat list. */
void kill_chat(struct chat_t *the_chat,struct chat_t **clist)
{
	struct chat_t *tmp=*clist;
	
	if (*clist==the_chat)
	{
		*clist=tmp->next;
	}
	else
	{
		while(tmp->next!=the_chat)
		{
			tmp=tmp->next;
			DEBUG0("Checking...\n");
		}
		tmp->next=the_chat->next;
	}
	if (the_chat->active)
	{
		close(the_chat->sockfd);
	}
	else
	{
		close(the_chat->oldsockfd);
		close(the_chat->ipc[1]);
	}
	close(the_chat->ipc[0]);
	waitpid(the_chat->childpid,NULL,0);
	free(the_chat);
}
	
/* Function: Checks the chat list for new data. */	
void check_chat(struct chat_t **clist)
{
	fd_set fdvar;
	struct timeval tval={0,0};
	char ch,buf[BIG_BUFFER_SIZE+1],*cp,*lp;
	int rnum,idx,motdfd,motdsize,biggest;
	struct chat_t *tmp=*clist;
	struct childmsg_t m;		

	for (tmp=*clist;tmp;tmp=tmp->next)
	{
		if (!tmp->active&&(tmp->sockfd=accept(tmp->oldsockfd,(struct sockaddr *) &tmp->cli_addr,&tmp->clilen))==-1)
		{
			/* Something went wrong. Or maybe right. */
			if (errno==EWOULDBLOCK||errno==EAGAIN)
			{
				/* Okay, we'll just have to wait. */
				if (time(NULL)>(tmp->stime+cfg.dcctimeout))
				{
					
					kill_chat(tmp,clist);
					return;
				}
			}
			else
			{
				/* This is serious. Better kill it. */				
				kill_chat(tmp,clist);
				return;
			}
		}
		else if (!tmp->active)
		{
			/* Connection established. */
			tmp->active=1;
			close(tmp->oldsockfd);
			/* Reset time counters */
			tmp->stime=tmp->itime=time(NULL);
			send_to_process(tmp,NUM,"%d",tmp->chatter->dcclimit);
			send_to_process(tmp,AUTO,"%d %d",tmp->chatter->flags&F_AUTO?1:0,tmp->chatter->flags&F_RSUM?1:0);
#ifdef ENABLE_RATIOS
			send_to_process(tmp,RATIO,"RATIO %d",tmp->chatter->ratio);
			send_to_process(tmp,RATIO,"UL %llu",tmp->chatter->ul);
			send_to_process(tmp,RATIO,"DL %llu",tmp->chatter->dl);
			send_to_process(tmp,RATIO,"CRED %llu",tmp->chatter->cr);
#endif
			if (tmp->chatter->flags&F_MOTD)
			{
				/* Say something nice... */
				if ((motdfd=open(cfg.motdfile,O_RDONLY))<0)
				{
					uprintf(tmp,"Hi there! I am sorry, but my MOTD file is missing...\n");
				}
				else
				{
					motdsize=read(motdfd,buf,BIG_BUFFER_SIZE-1);
					buf[motdsize-1]='\n';
					buf[motdsize]='\0';
					for (cp=buf;*cp!='\0';)
					{
						lp=cp;
						while(*cp!='\0'&&*cp!='\n') cp++;
						if (*cp!='\0')
						{
							*cp++='\0';
							uprintf(tmp,"%s\n",lp);
						}
					}
					close(motdfd);
				}
			}
		}
	}

	/* Connection is established, check for input. */
	FD_ZERO(&fdvar);
	biggest=0;
	for (tmp=*clist;tmp;tmp=tmp->next)
	{
		if (tmp->active)
		{
			FD_SET(tmp->sockfd,&fdvar);
			if (tmp->sockfd>biggest) biggest=tmp->sockfd;
			FD_SET(tmp->ipc[0],&fdvar);
			if (tmp->ipc[0]>biggest) biggest=tmp->ipc[0];
		}
	}

	if (select(biggest+1,&fdvar,(fd_set *)0,(fd_set *)0,&tval))
	{
		for (tmp=*clist;tmp;tmp=tmp->next)
		{
			if (FD_ISSET(tmp->sockfd,&fdvar))
			{
				idx=0;
				do
				{
					rnum=read(tmp->sockfd,&ch,1);
					if (!rnum)
					{
						/* No point going on */
						kill_chat(tmp,clist);
						return;
					}
					/* Stupid PiRCH sends "\r\n" to mark the */
					/* end of line instead of the normal "\n". */
					/* Thanks to Brainiac for discovering this. */
					if (ch!='\n'&&ch!='\r')
					{
						buf[idx++]=ch;
					}
				}
				while (ch!='\n');
				buf[idx]='\0';
				parse_command(tmp,buf);
			}
			if (FD_ISSET(tmp->ipc[0],&fdvar))
			{
				if (!read(tmp->ipc[0],&m,sizeof(struct childmsg_t)))
				{
					/* Our child is probably dead. */
					uprintf(tmp,"Closing connection: Child process died, please reconnect.\n");
					kill_chat(tmp,clist);
					return;
				}
				parse_child_message(tmp,m.type,m.msg);
			}
		}
	}
}

/* Function: Check if idle time has expired. */
void check_idle_time(struct chat_t **clist)
{
	struct chat_t *tmp;
	for (tmp=*clist;tmp;tmp=tmp->next)
	{
		if ((tmp->chatter->level<MASTER_USER)&&(!tmp->filenum)&&(time(NULL)>(tmp->stime+cfg.chattimeout)))
		{
			uprintf(tmp,"Okay, you've had your %d minute%s of fame. Bye!\n",cfg.chattimeout/60,cfg.chattimeout/60!=1?"s":"");
			kill_chat(tmp,clist);
			return;
		}
	}
}

/* Function: Removes chats marked for termination. */
void remove_marked_chats(struct chat_t **clist)
{
	struct chat_t *tmp;

	for (tmp=*clist;tmp;tmp=tmp->next)
	{
		if (tmp->terminate)
		{
			kill_chat(tmp,clist);
			return;
		}
	}
}

/* Function: Sends a message to a connected user. */
void uprintf(struct chat_t *chat,char *fmt,...)
{
	va_list ap;
	char buff[MSG_LEN+1];

	va_start(ap,fmt);
	vsprintf(buff,fmt,ap);
	va_end(ap);
	write(chat->sockfd,buff,strlen(buff));
}

/* Function: Sends a message to another process. If usr is NULL, then send to 'parent'.*/
void send_to_process(struct chat_t *usr,int type,char *fmt,...)
{
	struct childmsg_t m;
	va_list ap;
	m.type=type;
	va_start(ap,fmt);
	vsprintf(m.msg,fmt,ap);
	va_end(ap);
	write(usr?usr->ipc[0]:parent,&m,sizeof(struct childmsg_t));
	/* DEBUG3("send_to_process(%s,%d,%s)\n",usr?"child":"parent",m.type,m.msg); */
}


/* Function: Parses a message from the child, and acts appropriately. */
void parse_child_message(struct chat_t *usr,int message,char *msgtext)
{
	time(&usr->stime); /* Reset child idle time */
	switch (message)
	{
#if ENABLE_TAR
	case TAR:	
#endif
	case SEND:	send_dcc_offer(msgtext);
			break;
	case NUM:	sscanf(msgtext,"%d %d",&usr->filenum,&usr->queued);
			break;
	case LIST:
	case QLIST:
	case NEXT:	display_stuff(usr,msgtext);
			break;
	case RESUME:	send_to_server("PRIVMSG %s :\001DCC ACCEPT %s\001\n",usr->nick,msgtext);
			break;
#ifdef ENABLE_RATIOS
	case RATIO:	parse_child_ratio(usr,msgtext);
			break;
#endif
	default:
			break;
	}
}

/* Function: General function to display things to a user. */
void display_stuff(struct chat_t *usr,char *msgtext)
{
	char *usernick;
	struct user_t *tmpuser;
	struct chat_t *tmpchat;

	for (usernick=msgtext;*msgtext!=' ';msgtext++);
	*msgtext++='\0';

	if (!strcmp(usernick,"?"))
	{
		uprintf(usr,"%s\n",msgtext);
		return;
	}
	
	if ((tmpuser=get_user(usernick,&users)))
	{
		if ((tmpchat=find_chat(tmpuser,&chatlist)))
		{
			uprintf(tmpchat,"%s\n",msgtext);
		}
	}
}	

/* Function: Parses a ratio message from the child. */
#ifdef ENABLE_RATIOS
void parse_child_ratio(struct chat_t *usr,char *msgtext)
{
	char *cmd,*data;

	if (!(cmd=strtok(msgtext," "))||!(data=strtok(NULL," ")))
	{
		return;
	}
	if (!strcasecmp(cmd,"RATIO"))
	{
		sscanf(data,"%d",&usr->chatter->ratio);
	}
	else if (!strcasecmp(cmd,"UL"))
	{
		sscanf(data,"%llu",&usr->chatter->ul);
	}
	else if (!strcasecmp(cmd,"DL"))
	{
		sscanf(data,"%llu",&usr->chatter->dl);
	}
	else if (!strcasecmp(cmd,"CRED"))
	{
		sscanf(data,"%llu",&usr->chatter->cr);
	}
}
#endif

