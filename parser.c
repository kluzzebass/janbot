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
 * $Id: parser.c,v 1.5 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>


/* Function: Reads a line from the server and prepares it for parsing. */
int read_server_line(char *buff)
{
	int index=0;
	char tmp;
	fd_set fdvar;
	struct timeval tval={0,0};
	
	FD_ZERO(&fdvar);
	FD_SET(sconn.servfd,&fdvar);
	if (select(sconn.servfd+1,&fdvar,(fd_set *)0,(fd_set *)0,&tval))
	{
		time(&sconn.lastrecv);
		while(read(sconn.servfd,&tmp,1)>0)
		{
			if (tmp!='\n')
			{
				buff[index++]=tmp;
			}
			else
			{
				buff[index]='\0';
				return(index);
			}
		}
		return(-1);
	}
	buff[0]='\0';
	return (0);
}

/* Function: Parses a prepared line from the server. */
int parse_server_line(char *buff)
{
	char *orig,*cmd,*dest,*args,*tmp;

	for (tmp=buff;(*tmp!='\r')&&(*tmp!='\0');tmp++);
	*tmp='\0';
	
	if (buff[0]==':')
	{
		orig=++buff;
		for (args=orig;(*args!=' ')&&(*args!='\0');args++);
		if (*args==' ') *args='\0';
		cmd=++args;
		for (;(*args!=' ')&&(*args!='\0');args++);
		if (*args==' ') *args='\0';
		dest=++args;
		for (;(*args!=' ')&&(*args!='\0');args++);
		if (*args==' ') *args='\0';
		if (*++args==':') args++;
				
		if (strchr(orig,'!'))
		{
			parse_client_message(orig,cmd,args);
		}
		else
		{
			parse_server_message(orig,cmd,args);
		}		
	}
	else
	{
		args=buff;
		while ((*args!=' ')&&(*args!='\0')) args++;
		*args++='\0';
		if (!strcasecmp(buff,"PING"))
		{
			send_to_server("PONG :%s\n",cfg.nick);
			time(&sconn.lastping);
		}
		else
		if (!strcasecmp(buff,"ERROR"))
		{
			close(sconn.servfd);
			DEBUG0("ERROR: Connection closed from server.\n");
			log(SERV,"SERVER ERROR \"%s\"",args);
		}
		else
		if (!strcasecmp(buff,"NOTICE"))
		{
			log(SERV,"SERVER NOTICE \"%s\"",args);
		}
		/* There are lots of more server messages, but there's */
		/* no point in trying to handle them. Most of it is */
		/* channel related information, which we do not care about. */
		else log(SERV,"SERVER ??? \"%s\"",args);
	}
	return(0);
}

/* Function: Parses a server message. */
void parse_server_message(char *orig,char *cmd,char *args)
{
	int numeric;
	if (sscanf(cmd,"%d",&numeric))
	{
		switch (numeric)
		{
		case 302:
			parse_userhost_reply(args);
			break;
		case 433:
			close(sconn.servfd);
			DEBUG0("Bugger. Nickname in use.\n");
			log(SERV,"Nickname in use: %s.",cfg.nick);
			break;
		default:
			DEBUG2("SrvMsg %.3d: %s\n",numeric,args);
			break;
		}
	}
	else /* Handle nonnumerical messages. */
	{
		/* We don't really care about alphanumeric messages. */
		DEBUG2("SrvMsg %s: %s\n",cmd,args);
	}
}

/* Function: Parses a message from a client. */
void parse_client_message(char *orig,char *cmd,char *args)
{
	if (*args=='\001')
	{
		args++;
		*strrchr(args,'\001')='\0';
		if (!strcasecmp(cmd,"PRIVMSG"))
		{
			parse_ctcp(orig,args);
		}
		else
		if (!strcasecmp(cmd,"NOTICE"))
		{
			parse_ctcp_reply(orig,args);
		}
	}
	else
	{
		if (!strcasecmp(cmd,"PRIVMSG"))
		{
			parse_privmsg(orig,args);
		}
		else
		if (!strcasecmp(cmd,"NOTICE"))
		{
			parse_notice(orig,args);
		}
	}
}

/* Function: Parses a MSG from a client. */
void parse_privmsg(char *orig, char *args)
{
	/* This bot does not like to be msg'ed. Maybe we'll just recite */
	/* some poetry or something, to annoy the sender of the msg. */
	DEBUG2("PRIVMSG from %s received: %s\n",getnick(orig),args);
	log(MSG,"PRIVMSG from %s: %s",orig,args);
}

/* Function: Parses a NOTICE from a client. */
void parse_notice(char *orig, char *args)
{
	/* We do not act on NOTICE, it could cause loops. */
	DEBUG2("NOTICE from %s received: %s\n",getnick(orig),args);
	log(MSG,"NOTICE from %s: %s",orig,args);
}

/* Function: Parses and executes a command from a user. */
void parse_command(struct chat_t *tmp,char *buffer)
{
	int index;
	char **av;

	if (!(av=parse_line(buffer)))
	{
		DEBUG0("Something broke.\n");
		uprintf(tmp,"Error in command line.\n");
		return;
	}

	if (!av[0])
	{
		uprintf(tmp,toolow());
		free_argv(av);
		return;
	}

	/* Log this command... */
	log(CMD,"<%s> %s",tmp->chatter->nick,buffer);

	for (index=0;(cmdlist[index].name)&&(strcasecmp(cmdlist[index].name,av[0]));index++);
	if (cmdlist[index].name)
	{
		if (cmdlist[index].level>tmp->chatter->level)
		{
			uprintf(tmp,toolow());
		}
		else if (!cmdlist[index].func(tmp,av+1))
		{
			showusage(tmp,av[0]);
		}
	}
	else
	{
		/* Some gimpy answer will be returned here. */
		uprintf(tmp,toolow());
	}
	/* Finally, get rid of the command string vector. */
	free_argv(av);
	
	/* Reset user and global idle time. Global idle time is mostly used
	   by CTCP FINGER but also in the STATUS command. */
	tmp->itime=tmp->stime=idle_time=time(NULL);
}

