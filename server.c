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
 * $Id: server.c,v 1.2 1998/11/03 04:41:06 kluzz Exp $
 */


#include <janbot.h>


struct server_t *current_server=NULL;
struct servconn_t sconn;
struct servmsg_t *msgqueue=NULL;



/* Function: Connects to specified server, returns 0 upon success. */
int server_connect(struct server_t *desc)
{
	struct hostent	*temphp;
	unsigned long	inaddr;

	close(sconn.servfd);

	memset((char *)&sconn.serv_addr,0,sizeof(sconn.serv_addr));
	sconn.serv_addr.sin_family=AF_INET;

	DEBUG2("Connecting to server: %s:%d\n",desc->name,desc->port);

	if ((inaddr=inet_addr(desc->name))!=INADDR_NONE)
	{
		/* We have an IP address. */
		memcpy((char *)&sconn.serv_addr.sin_addr,(char *)&inaddr,sizeof(inaddr));
	}
	else
	{
		if (!(temphp=gethostbyname(desc->name)))
		{
			/* No host. That sux. */
			return(-1);
		}
		memcpy((char *)&sconn.serv_addr.sin_addr,temphp->h_addr,temphp->h_length);
	}
	
	sconn.serv_addr.sin_port=htons(desc->port);

	if ((sconn.servfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		log(CRAP,"Socket creation error.");
		return(0);
	}

	if (connect(sconn.servfd,(struct sockaddr *)&(sconn.serv_addr),sizeof(sconn.serv_addr))<0)
	{
		DEBUG0("Server connection failed.\n");
		log(SERV,"Connection to %s:%d.",desc->name,desc->port);
		close(sconn.servfd);
		return(0);
	}	
	else
	{
		DEBUG1("Connection to server %s established.\n",desc->name);
		log(SERV,"Connected to server %s:%d",desc->name,desc->port);
	}
	return(1);
}

/* Function: Initializes the server connection with client data. */
int server_init()
{
	/* This should definitely be improved. */

	send_to_server("NICK %s\n",cfg.nick);
	send_to_server("USER %s %s %s :%s\n",getpwuid(getuid())->pw_name,hostname,hostname,cfg.userinfo);
	time(&sconn.lastrecv);
	return(0);
}


/* Function: Returns a pointer to the next server in serverlist. */
struct server_t *next_server()
{
	if ((!current_server)||(!current_server->next))
	{
		current_server=cfg.servlist;
	}
	else if (cfg.walking)
	{
		current_server=current_server->next;
	}
	return(current_server);
}

/* Function: Adds a new server to the server list. */
void add_server(char sname[SERVER_LEN],int sport)
{
	struct server_t *tmp,*p;
	if ((tmp=(struct server_t *)malloc(sizeof(struct server_t))))
	{
		memset(tmp,0,sizeof(struct server_t));

		strcpy(tmp->name,sname);
		tmp->port=sport;
		tmp->next=NULL;
		
		if (!cfg.servlist)
		{
			cfg.servlist=tmp;
		}
		else
		{
			for (p=cfg.servlist;p->next;p=p->next);
			p->next=tmp;
		}
	}
}

/* Function: Removes a server from the server list. */
char *del_server(int num)
{
	int cnt;
	struct server_t *p,*q;
	static char reply[NAME_MAX];

	if (!cfg.servlist)
	{
		sprintf(reply,"Unable to delete server number \002%d\002.\n",num);
		return(reply);
	}	
	if (num==1)
	{
		p=cfg.servlist;
		cfg.servlist=p->next;
		sprintf(reply,"Deleted server \002%s:%d\002.\n",p->name,p->port);
		if (p==current_server) close(sconn.servfd);
		free(p);
		return(reply);
	}
	for (p=cfg.servlist->next,q=cfg.servlist,cnt=2;(p->next)&&(cnt<num);q=p,p=p->next,cnt++);
	if (cnt!=num)
	{
		sprintf(reply,"Unable to delete server number \002%d\002.\n",num);
	}
	else
	{
		q->next=p->next;
		sprintf(reply,"Deleted server \002%s:%d\002.\n",p->name,p->port);
		if (p==current_server) close(sconn.servfd);
		free(p);
	}
	return(reply);
}

/* Function: Puts a message on the servermessage queue. (Uh, not!) */
void send_to_server(char *fmt,...)
{
	va_list ap;
	char msg[MSG_LEN+1];
	
	va_start(ap,fmt);
	vsprintf(msg,fmt,ap);
	va_end(ap);
	write(sconn.servfd,msg,strlen(msg));
	time(&sconn.lastping);
}
