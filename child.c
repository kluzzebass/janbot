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
 * $Id: child.c,v 1.5 1998/11/08 23:36:06 kluzz Exp $
 */


#include <janbot.h>

int parent; /* Descriptor for communication with the parent process */

#ifdef ENABLE_RATIOS
int ratio;
unsigned long long ul;
unsigned long long dl;
unsigned long long cred;
#endif /* ENABLE_RATIOS */


/* Function: Prepares a child process, frees up memory. */
void init_child(struct chat_t *user)
{
	struct chat_t *tmp;
	int idx;
	
	parent=user->ipc[1];

#ifdef ENABLE_DEBUG
	signal(SIGTERM,child_signal_handler);
	signal(SIGINT,child_signal_handler);
	signal(SIGSEGV,child_signal_handler);
	signal(SIGBUS,child_signal_handler);
	signal(SIGQUIT,child_signal_handler);
#else
	signal(SIGTERM,SIG_DFL);
	signal(SIGINT,SIG_DFL);
	signal(SIGSEGV,SIG_DFL);
	signal(SIGBUS,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
#endif /* ENABLE_DEBUG */
	signal(SIGHUP,SIG_IGN);

	/* We ignore the SIGPIPE signal to prevent the process from */
	/* dying when people close DCCs using the /DCC CLOSE command. */
	signal(SIGPIPE,SIG_IGN);


	fcntl(parent,F_SETFL,O_NONBLOCK);

	/* Close some file descriptors. */
	close(user->ipc[0]);
	fclose(userfile);
	close(sconn.servfd);
	fclose(logfile);

	free_users(&users);
	while ((tmp=chatlist))
	{
		kill_chat(tmp,&chatlist);
	}

	for (idx=0;idx<p_argc;idx++)
	{
		memset(p_argv[idx],0,strlen(p_argv[idx]));
	}	
	sprintf(p_argv[0],"%s%s",cfg.nick,DCC_CHAT_CHILD);

	dcclist=NULL;
}

/* Function: Do whatever the child process needs to do. */
int process_child()
{
	fd_set fdvar;
	struct timeval tval={0,0};
	struct childmsg_t m;
	int idx;

	/* Check with our parent. ("Mommy, can I go out and play?") */
	
	FD_ZERO(&fdvar);
	FD_SET(parent,&fdvar);

	if (select(parent+1,&fdvar,(fd_set *)0,(fd_set *)0,&tval))
	{
		/* New input arrived. */
		idx=0;

		if (!read(parent,&m,sizeof(struct childmsg_t)))
		{
			kill_child();
			return(0);
		}		
		parse_parent_message(m.type,m.msg);
	}
	check_dcc();
	return(1);
}


void kill_child()
{
	struct dcc_t *tmp;
	struct dccqueue_t *dq;

	/* No connection. ("Mom! Where are you!?") */
	close(parent);
	/* Get rid of dcc's. (Well, not yet.) */

	for (tmp=dcclist;tmp;tmp=tmp->next)
	{
		close(tmp->sockfd);
		fclose(tmp->file);
		put_on_deathrow(tmp);
	}
	clear_deathrow();	

	for (dq=dccqueue;dccqueue;dq=dccqueue)
	{
		dccqueue=dq->next;
		free(dq);
	}	
			
	/* Commit suicide. */
}

void parse_parent_message(int message,char *msgtext)
{
	char **mv;
	
	if (!(mv=parse_line(msgtext)))
	{
		DEBUG0("child.c: Error parsing parent message.\n");
		return;
	}

	switch (message)
	{
	case SEND:
			if (autodcc&&(dcc_count()<dcclimit)) process_outgoing_file(mv,D_SEND);
			else put_on_dccqueue(D_SEND,mv);
			break;
#if ENABLE_TAR
	case TAR:
			if (autodcc&&(dcc_count()<dcclimit)) process_outgoing_file(mv,D_TAR);
			else put_on_dccqueue(D_TAR,mv);
			break;
#endif
	case GET:
			if (autodcc&&(dcc_count()<dcclimit)) process_incoming_file(mv);
			else put_on_dccqueue(D_GET,mv);
			break;
	case NUM:
			sscanf(mv[0],"%d",&dcclimit);
			break;
	case LIST:
			show_dcc_list(mv);
			break;
	case QLIST:
			show_dcc_queue(mv);
			break;
	case KILL:
			remove_dcc(mv);
			break;
	case QKILL:
			remove_queued(mv);
			break;
	case AUTO:
			sscanf(mv[0],"%d",&autodcc);
			sscanf(mv[0],"%d",&resume);
			break;
	case NEXT:	process_next(mv);
			break;
	case RESUME:	process_resume(mv);
			break;
#ifdef ENABLE_RATIOS
	case RATIO:	parse_parent_ratio(mv);
			break;
#endif
	default:
			send_to_process(NULL,ERROR,"This does not compute.");
			break;
	}
	free_argv(mv);
}


#ifdef ENABLE_DEBUG
/* Function: Handle some of the possible signals and print it out. */
void child_signal_handler(int sig)
{
	char *signame;
	signal(SIGTERM,SIG_IGN);
	signal(SIGINT,SIG_IGN);
	signal(SIGSEGV,SIG_IGN);
	signal(SIGBUS,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	
	switch(sig)
	{
	case SIGTERM:
		signame="SIGTERM";
		break;
	case SIGINT:
		signame="SIGINT";
		break;
	case SIGSEGV:
		signame="SIGSEGV";
		break;
	case SIGBUS:
		signame="SIGBUS";
		break;
	case SIGQUIT:
		signame="SIGQUIT";
		break;
	default:
		signame="Unknown";
		break;
	}
	DEBUG2("Chat child (pid %d) signal: %s\n",(int)getpid(),signame);
	_exit(0);
}
#endif /* ENABLE_DEBUG */

/* Function: Parses the RATIO info. */
#ifdef ENABLE_RATIOS
void parse_parent_ratio(char **mv)
{
	if (!mv[0]||!mv[1]) return;

	if (!strcasecmp(mv[0],"RATIO"))
	{
		sscanf(mv[1],"%d",&ratio);
	}
	else if (!strcasecmp(mv[0],"UL"))
	{
		sscanf(mv[1],"%llu",&ul);
	}
	else if (!strcasecmp(mv[0],"DL"))
	{
		sscanf(mv[1],"%llu",&dl);
	}
	else if (!strcasecmp(mv[0],"CRED"))
	{
		sscanf(mv[1],"%llu",&cred);
	}
}
#endif
