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
 * $Id: janbot.c,v 1.3 1998/11/07 19:14:11 kluzz Exp $
 */


#include <janbot.h>

time_t bot_uptime;
time_t idle_time;
int background=1;

/* For publishing the argument list */
int p_argc;
char **p_argv;

/* Info about ourselves. */
struct in_addr hostaddr;
char hostname[256];
unsigned long local_ip;
struct hostent *local_hp;


/* Function: Runs the show. */
int main(int argc,char **argv)
{
	int idx,manual_useradd=0;
#ifdef _BSD_
	int ttyfd;
#endif
	char *cfgfile=NULL;

	/* Others might want to use this. */
	p_argc=argc;
	p_argv=argv;

	/* Seed the random generator. The PID is likely not to be the same */
	/* twice, so it's fairly safe to do this. It doesn't really matter. */
	srand((int)getpid());

	/* Let's find our own ip, might get handy for DCC... */
	gethostname(hostname,sizeof(hostname));	
	if ((local_hp = gethostbyname(hostname)) != NULL)
	{
		memcpy((char *)&hostaddr,local_hp->h_addr,sizeof(hostaddr));
		/* Okay, store ip in network order, to save time. */
		local_ip=htonl(hostaddr.s_addr);
	}

	/* Let's parse the command line. */
	for (idx=1;idx<argc;idx++)
	{
		if ((argv[idx][0]=='-')&&(strlen(argv[idx])==2))
		{
			switch (argv[idx][1])
			{
#ifdef ENABLE_DEBUG
			case 'd':
				background=0;
				traceflag++;
				break;
#endif /* ENABLE_DEBUG */
			case 'l':
				if (argv[++idx])
				{
					cfgfile=expand_tilde(argv[idx]);
				}
				else
				{
					show_cli_help();
					exit(1);
				}
				break;
			case 'm':
				manual_useradd=1;
				break;
			default:
				show_cli_help();
				exit(1);
				break;
			}
		}
	}

	init_config(cfgfile,NULL);

	/* Let's switch to our home directory... */
	if (chdir(cfg.bothome)==-1)
	{
		fprintf(stderr,"Unable to switch home directory!\n");
		exit(1);
	}

	umask(0); /* This allows us to create files with any permissions. */

	if (!initusers(manual_useradd))
	{
		fprintf(stderr,"Unable to open user database. Try parameter -m.\n");
		exit(1);
	}

	if (!(logfile=fopen(cfg.logfile,"a")))
	{
		fprintf(stderr,"Unable to open logfile: %s\n",cfg.logfile);
		exit(1);
	}
	log(ALL,"*** LOG STARTED ***");

	if (manual_useradd)
	{
		enter_new_user();
		free_users(&users);
		exit(0);
	}

	for (idx=0;idx<argc;idx++)
	{
		memset(argv[idx],0,strlen(argv[idx]));
	}	
	sprintf(argv[0],"%s%s",cfg.nick,MAIN_PROCESS);

	read_users(&users);

	/* Set up some signal handling. */
	signal(SIGTERM,signal_handler);
	signal(SIGINT,signal_handler);
#ifndef ENABLE_DEBUG
	signal(SIGSEGV,signal_handler);
	signal(SIGBUS,signal_handler);
#endif /* ENABLE_DEBUG */
	signal(SIGQUIT,signal_handler);
	signal(SIGHUP,sighup_handler);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGCHLD,SIG_IGN); /* We handle these elsewhere. */
#ifdef SIGTTOU
	signal(SIGTTOU,SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN,SIG_IGN);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif

	/* Fork to background if not in debug mode */
	daemonize();

	/* We're up. */
	bot_uptime=idle_time=time(NULL);


	/* The main loop. */
	do
	{
		while(!server_connect(next_server()));
		server_init();
	}
	while (do_bot_things());

	close(sconn.servfd);
	fclose(userfile);
	fclose(logfile);
	free_users(&users);
	return(0);
}

int do_bot_things()
{
	char buf[MSG_LEN+1];
	while (read_server_line(buf)>-1)
	{
		if (strlen(buf)>0) parse_server_line(buf);

		if (cfg.pingtimeout&&(sconn.lastrecv+cfg.pingtimeout)<time(NULL))
		{
			log(SERV,"Nothing received from server in %d seconds; switching server.",cfg.pingtimeout);
			DEBUG0("Ping timeout.\n");
			close(sconn.servfd);
		}

		/* Active PING is maybe not too smart, but it works. */
		if ((sconn.lastping+cfg.pingtimeout/2)<time(NULL))
		{
			send_to_server("PING :%s\n",cfg.nick);
			time(&sconn.lastping);
		}

		check_chat(&chatlist);
		check_idle_time(&chatlist);
		remove_marked_chats(&chatlist);
		write_scheduled(&users);

		ms_delay(10); 	/* This will keep the bot from surfing */
				/* on the top cpu usage list... =) */
	}
	close(sconn.servfd);
	return(cfg.reconnect);
}


void show_cli_help()
{
	fprintf(stderr,"Usage: %s [options]\n\n",*p_argv);
#ifdef ENABLE_DEBUG
	fprintf(stderr,"   -d               Run in debug mode.\n");
#endif /* ENABLE_DEBUG */
	fprintf(stderr,"   -l <configfile>  Use another config file.\n");
	fprintf(stderr,"   -m               Manually add a new user.\n");
}

/* Function: Handle some of the possible signals, and inform the users. */
RETSIGTYPE signal_handler(int sig)
{
	struct chat_t *tmp;
	char buf[MSG_LEN+1];

	signal(SIGTERM,SIG_IGN);
	signal(SIGINT,SIG_IGN);
#ifndef ENABLE_DEBUG
	signal(SIGSEGV,SIG_IGN);
	signal(SIGBUS,SIG_IGN);
#endif /* ENABLE_DEBUG */
	signal(SIGQUIT,SIG_IGN);
	
	switch(sig)
	{
	case SIGTERM:
		sprintf(buf,"I have been brutally SIGTERM'ed. Terminating connection.\n");
		log(ALL,"Exiting on signal: SIGTERM");
		break;
	case SIGINT:
		sprintf(buf,"I have been brutally SIGINT'ed. Terminating connection.\n");
		log(ALL,"Exiting on signal: SIGINT");
		DEBUG0("Caught SIGINT. Please wait while shutting down.\n");
		break;
#ifndef ENABLE_DEBUG
	case SIGSEGV:
		sprintf(buf,"I seem to have caused a SIGSEGV. Terminating connection.\n");
		log(ALL,"Exiting on signal: SIGSEGV");
		break;
	case SIGBUS:
		sprintf(buf,"I seem to have encountered a SIGBUS. Terminating connection.\n");
		log(ALL,"Exiting on signal: SIGBUS");
		break;
#endif /* ENABLE_DEBUG */
	case SIGQUIT:
		sprintf(buf,"I have been brutally SIGQUIT'ed. Terminating connection.\n");
		log(ALL,"Exiting on signal: SIGQUIT");
		break;
	default: /* Eh, could this ever happen? */
		sprintf(buf,"Caught an unknown signal. Terminating connection.\n");
		log(ALL,"Exiting on unknown signal.");
		break;
	}

	/* Only write the userlist when a 'safe' signal is received. */
	switch(sig)
	{
	case SIGTERM:
	case SIGINT:
	case SIGQUIT:
		write_users(&users);
		break;
	default:
		break;
	}
	
	/* Send termination messages to all the users. */
	for (tmp=chatlist;tmp;tmp=tmp->next)
	{
		write(tmp->sockfd,buf,strlen(buf));
	}
	log(ALL,"***  LOG ENDED  ***");
	exit(0);
}

/* Function: Handles the SIGHUP signal and rehashes the config file. */
RETSIGTYPE sighup_handler(int sig)
{
	signal(SIGHUP,SIG_IGN);
	log(SIG,"Caught signal SIGHUP, rehashing config file.");
	init_config(NULL,NULL);
	signal(SIGHUP,sighup_handler);
#if RETSIGTYPE == void
	return;
#else
	return(0);
#endif
}	

void daemonize()
{
	/* If not debug mode, fork to background. */
	if (background&&fork())
	{
		exit(0);
	}
	else
	{
		setsid();
		if (background&&fork())
		{
			exit(0);
		}
		else if (background)
		{
			close(0);
			close(1);
			close(2);
		}
	}
}
