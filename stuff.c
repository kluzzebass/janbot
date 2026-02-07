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
 * $Id: stuff.c,v 1.6 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>

#ifdef ENABLE_DEBUG
int traceflag=0;
int function_level=0;
#endif /* ENABLE_DEBUG */

FILE *logfile; /* Descriptor for the log file. */


/* These comments are from the previous JanBot, the ircII bot. */
char *toolowlist[]={
"Are you insane?\n",
"You fail to invent a new command.\n",
"Maybe you should try HELP?\n",
"The hideous lag monster swallows your command. Try again.\n",
"Yes, and may banana peels fall out your butt.\n",
"Hmmm... I need more exact instructions.\n",
"You just called me a bastard, didn't you?!\n",
"You really need some spell amelioration...\n",
"My language is English, what is your?\n",
"?oot era uoy ebyam ,sdrawkcab gniklat m'I\n",
"There's only one word for people like you: Supercalifragilisticexpialidocious.\n",
"Is this a trick question?\n",
"Gee, your spelling is so bad you must be from Germany!\n",
"Huh?\n",
"Oh yeah, you and what army?\n",
"Exsqueeze me? Baking powder?\n",
"I'm not taking any orders from you!\n",
"You're damned if you do and you're damned if you don't!\n",
"There should be a neat comment here, but I can't think of one right now.\n",
"[Funny comment goes here]\n",
"I can't believe you said that!\n",
"What we have here is a failure to communiucate.\n",
"Shaddap, or I'll smack you!\n",
"Oh, sorry, I wasn't paying attention...\n",
"You know it's funny, I heard that exact same comment the other day.\n",
"Your puny hacking attempt has been logged by the FBI.\n",
"Sorry, I don't do requests.\n",
"Commands, who needs them?!\n",
"Something successfully did not happen.\n",
"Oh yeah? What do I look like, a psychic? You want me to guess what you want?\n",
"Go away, can't you see we're closed?!\n",
"Please insert coin.\n",
"Don't make me resort to violence.\n",
"Don't make me laugh.\n",
"Stop sex talking me!\n",
"Please keep your ass tight.\n",
"Duc Duc Gnouf?\n",
"Whahaha!!\n",
"Oink... ddup dup!\n",
"Run that by me again, please...\n",
"You didn't say \"please\".\n",
"Your command quota has been exceeded.\n",
"What is it, man?!\n",
"What?!\n",
"What do you mean?\n",
"Qu�?",
NULL
};




/* Function: waits for a given number of milliseconds, then returns. */
void ms_delay(int msec)
{
	struct timeval tval={0,(long)(msec*1000)};
	select(0,(fd_set *)0,(fd_set *)0,(fd_set *)0,&tval);
}

/* Function: Returns nickname from nick!user@host id string. */
char *getnick(char *origin)
{
	static char thenick[NICK_LEN+1];
	char *tmp,*tmp2;
	for (tmp=origin,tmp2=thenick;*tmp!='!';tmp++) *(tmp2++)=*tmp;
	*tmp2='\0';
	return(thenick);
}

/* Function: Returns user@host from nick!user@host id string */
char *getuserhost(char *origin)
{
	static char theuserhost[USERHOST_LEN+1];
	char *tmp;
	for (tmp=origin;*tmp!='!';tmp++);
	strcpy(theuserhost,++tmp);
	return(theuserhost);
}

/* Function: Returns a pointer to a funny comment. :) */
char *toolow()
{
	int index;
	for (index=0;toolowlist[index];index++);
	return(toolowlist[rand()%index]);
}

/* Function: Converts a time_t to a string representation. */
char *convert_date(time_t t)
{
	static char date[18];
	struct tm *ltm;
	char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	
	if (!(ltm=localtime(&t)))
	{
		strcpy(date,"??? ?? ???? ??:??");
	}
	else
	{
		sprintf(date,"%s %2d %4d %.2d:%.2d",months[ltm->tm_mon],ltm->tm_mday,1900+ltm->tm_year,ltm->tm_hour,ltm->tm_min);
	}
	return(date);
}


/* Function: Converts idletime into hours:minutes:seconds */
char *convert_idle(time_t t)
{
	static char idle[12];
	if ((t/3600)<24)
	{
		sprintf(idle,"%.2u:%.2u:%.2u",(unsigned int)t/3600,(unsigned int)(t/60)%60,(unsigned int)t%60);
	}
	else
	{
		/* Almost only priviliged users stay on this long. */
		sprintf(idle,"%.7ud",(unsigned int)t/(3600*24));
	}
	return(idle);
}

/* Function: Converts uptime into a suitable format */
char *convert_uptime(time_t t)
{
	static char upt[20];
	if (t/3600/24)
	{
		sprintf(upt,"%ud %uh %um",(unsigned int)t/3600/24,(unsigned int)(t/3600)%24,(unsigned int)(t/60)%60);
	}
	else if (t/3600)
	{
		sprintf(upt,"%uh %um",((unsigned int)t/3600)%24,(unsigned int)(t/60)%60);
	}
	else
	{
		sprintf(upt,"%um",(unsigned int)(t/60)%60);
	}
	return(upt);
}


/* Function: Creates a suitable hostmask from a hostname or ip. */
char *create_hostmask(char *h)
{
	static char mask[USERHOST_LEN+1];
	int c=0,v;
	char *t[10]; /* This is hopefully enough. */
	
	do
	{
		if (!c)
		{
			t[c]=strtok(h,".");
		}
		else
		{
			t[c]=strtok(NULL,".");
		}
	}
	while (t[c++]);
	c--;	

	if (c==4) /* This could be an ip address. */
	{
		if ((sscanf(t[0],"%d",&v))&&(sscanf(t[1],"%d",&v))&&(sscanf(t[2],"%d",&v))&&(sscanf(t[3],"%d",&v)))
		{
			/* It is an ip address. */
			sprintf(mask,"%s.%s.%s.*",t[0],t[1],t[2]);
			return(mask);
		}
	}
	if (c==2) /* We won't mask a 2 segment hostname. */
	{
		sprintf(mask,"%s.%s",t[0],t[1]);
		return(mask);
	}
	if (c==1) /* We definitely won't mask a 1 segment hostname. */
	{
		strcat(mask,t[0]);
		return(mask);
	}
	strcpy(mask,"*");
	for (v=1;v<c;v++)
	{
		strcat(mask,".");
		strcat(mask,t[v]);
	}
	return(mask);
}

/* Function: Displays the short usage of a certain command. */
void showusage(struct chat_t *usr,char *cmd)
{
	int index;
	
	for (index=0;(cmdlist[index].name)&&(strcasecmp(cmdlist[index].name,cmd));index++);
	if (cmdlist[index].name)
	{
		uprintf(usr,"Usage: \002%s\002 %s\n",cmdlist[index].name,cmdlist[index].usage?cmdlist[index].usage:"");
	}
	else
	{
		uprintf(usr,"Unable to show usage: Unknown command.\n");
	}
}


/* Function: Displays help about a certain command. */
void showhelp(struct chat_t *usr,char *topic)
{
	int i,lvl;
	FILE *helpfile;
	char helpname[256],helpline[256];
	char **av;
	
	strcpy(helpname,cfg.bothome);
	strcat(helpname,"/");
	strcat(helpname,cfg.helpfile);
	
	if (!(helpfile=fopen(helpname,"r")))
	{
		uprintf(usr,"Unable to open help file!\n");
		return;
	}

	for (i=1;fgets(helpline,256,helpfile);i++)
	{
		if (helpline[strlen(helpline)-1]=='\n') helpline[strlen(helpline)-1]='\0';
		av=NULL;
		if (!(av=parse_line(helpline))||!av[1]||!av[2]||!sscanf(av[1],"%x",&lvl))
		{
			if (av[0]) uprintf(usr,"Error in help file, line %d: %s\n",i,helpline);
			if (av) free_argv(av);
			continue;
		}
		if (av[0]&&!strcasecmp(topic,av[0])&&((1<<usr->chatter->level)&lvl))
		{
			uprintf(usr,"%s\n",av[2]);
		}
		free_argv(av);
	}
	fclose(helpfile);
}


/* Function: Writes something to the logfile. */
void log(unsigned long logflag,char *fmt,...)
{
	va_list ap;
	struct tm *ltm;
	char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	time_t t;
	
	if (logflag&cfg.loglevel)
	{
		time(&t);
		if (!(ltm=localtime(&t)))
		{
			fprintf(logfile,"[??? ?? ??:??] ");
		}
		else
		{
			fprintf(logfile,"[%s %2d %.2d:%.2d:%.2d] ",months[ltm->tm_mon],ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
		}
		va_start(ap,fmt);
		vfprintf(logfile,fmt,ap);
		va_end(ap);
		fprintf(logfile,"\n");
		fflush(logfile);
	}	
}

/* Function: Returns a legal port number. */
unsigned int next_port()
{
	static unsigned int port;
	while (port++<1024);
	return(port);
}

/* Function: Determines the validity of a nick. */
int isvalidnick(char *n)
{
	int i;
	if (!n) return(0);
	for (i=0;n[i];i++)
	{
		if (	(n[i]<'A' || n[i]>'}') &&
			(n[i]<'0' || n[i]>'9') &&
			n[i]!='_' && n[i]!='-') return(0);
	}
	return(1);
}

/* Function: Checks a single file descriptor for incoming data. */
int readytoread(int fd)
{
	fd_set set;
	struct timeval t={0,0};

	FD_ZERO(&set);
	FD_SET(fd,&set);
	if (select(fd+1,&set,(fd_set *)0,(fd_set *)0,&t)<1) return(0);
	return(1);
}

/* Function: Allocates the requested amount of memory. */
void *mymalloc(size_t size)
{
	void *r;
	r=malloc(size);
	return(r);
}

/* Function: Quick fix for the strncpy() bug/feature. */
char *mystrncpy(char *dest,char *src,int n)
{
	dest[n]='\0';
	return(strncpy(dest,src,n));
}

/* Function: Frees a struct list_t. */
void lfree(struct list_t *l)
{
	/* Freeing a NULL pointer is a NOOP. */
	free(l->str);
	free(l);
}

/* Functions: Creates a struct list_t object, and inserts a string into it. */
struct list_t *ldup(char *s)
{
	struct list_t *t;
	
	if (!(t=(struct list_t *)malloc(sizeof(struct list_t)))) return NULL;
	if (!(t->str=strdup(s)))
	{
		free(t);
		return NULL;
	}
	t->next=NULL;
	return(t);
}

/* Function: Poor man's socketpair() for Cygnus' CDK package for Windows NT. */
#ifdef POOR_MANS_SOCKETPAIR
int socketpair(int d, int type, int protocol, int sv[2])
{
	struct sockaddr_in a,c;
	int fda,fdb,fdc,port,al;

	/* Other types of sockets are not supported. */
	if (d!=AF_UNIX||type!=SOCK_STREAM)
	{
		errno=EAFNOSUPPORT;
		return(-1);
	}
	/* And we only care about the IP protocol. */
	if (protocol)
	{
		errno=EPROTONOSUPPORT;
		return(-1);
	}
	/* Poor men use AF_INET. */
	if ((fda=socket(AF_INET,type,protocol))==-1)
	{
		return(-1);
	}
	if ((fdb=socket(AF_INET,type,protocol))==-1)
	{
		close(fda);
		return(-1);
	}
	if (fcntl(fda,F_SETFL,O_NONBLOCK)==-1)
	{
		close(fda);
		close(fdb);
		return(-1);
	}
	port=next_port();
	memset((char *)&a,0,sizeof(a));
	a.sin_family=AF_INET;
	a.sin_addr.s_addr=htonl(INADDR_ANY);
	a.sin_port=htons(port);
	if (bind(fda,(struct sockaddr *)&a,sizeof(a))<0)
	{
		close(fda);
		close(fdb);
		return(-1);
	}
	listen(fda,1);
	al=sizeof(a);
	if ((fdc=accept(fda,(struct sockaddr *)&a,&al))<0&&errno!=EWOULDBLOCK)
	{
		DEBUG0("Unable to accept first time.\n");
		close(fda);
		close(fdb);
		return(-1);
	}
	memset((char *)&c,0,sizeof(c));
	c.sin_family=AF_INET;
	c.sin_addr.s_addr=inet_addr("127.0.0.1");
	c.sin_port=htons(port);
	if (connect(fdb,(struct sockaddr *)&c,sizeof(c))<0)
	{
		DEBUG0("Unable to connect first time\n");
		close(fda);
		close(fdb);
		close(fdc);
		return(-1);
	}
	al=sizeof(a);
	if (fcntl(fda,F_SETFL,0)==-1)
	{
		close(fda);
		close(fdb);
		close(fdc);
		return(-1);
	}

	if ((fdc=accept(fda,(struct sockaddr *)&a,&al))<0)
	{
		DEBUG0("Unable to accept second time.\n");
		close(fda);
		close(fdb);
		close(fdc);
		return(-1);
	}
	sv[0]=fdb;
	sv[1]=fdc;
	return(0);
}
#endif /* POOR_MANS_SOCKETPAIR */
