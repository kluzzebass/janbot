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
 * $Id: userlist.c,v 1.4 1998/11/22 01:05:12 kluzz Exp $
 */

#include <janbot.h>

/* The list of users. */
struct user_t *users=NULL;

FILE *userfile=NULL; /* Descriptor for the userlist file. */

/* This is some oddstuff for the user add command. */
struct uhost_query uhquery;



/* Function: Adds a user to userlist, returns pointer to new user. */
struct user_t *add_user(char *nickname,struct user_t **ulist)
{
	struct user_t *tmp,*p,*prev;

	if (!isvalidnick(nickname)) return(NULL);
	if ((tmp=(struct user_t *)malloc(sizeof(struct user_t))))
	{
		memset((char *)tmp,0,sizeof(struct user_t));
		strcpy(tmp->nick,nickname);
		prev=*ulist;
		if ((!prev)||(strcasecmp(nickname,prev->nick)<=0))
		/* User list is empty, insert user at start. */
		{
			tmp->next=*ulist;
			*ulist=tmp;
		}
		else
		/* Userlist contains users, let's find the correct place. */
		{
			p=prev->next;
			while ((p)&&(strcasecmp(nickname,p->nick)>0))
			{
				prev=p;
				p=prev->next;
			}
			tmp->next=p;
			prev->next=tmp;
		}
	}
	else return(NULL);

	/* Let's put in some default values. */
	tmp->dcclimit=cfg.dcclimit;
	tmp->flags=cfg.defaultflags;
	tmp->userhost=NULL;
	tmp->alias=NULL;
	tmp->ratio=cfg.defaultratio;
	strcpy(tmp->dir,"/");
	return(tmp);
}
	

/* Function: Finds a user in userlist, returns pointer to user. */
struct user_t *get_user(char *nickname,struct user_t **ulist)
{
	struct user_t *tmp;
	tmp=*ulist;
	/* I could make this recursive. I love recursion. But I won't bother. */
	while ((tmp)&&strcasecmp(nickname,tmp->nick))
 	{
		tmp=tmp->next;
	}
	/* If we found a user, that's great, otherwise NULL is returned. */
	return(tmp);
}

/* Function: Find nick or alias in the userlist. */
struct user_t *get_alias(char *nickname,struct user_t **ulist)
{
	struct user_t *tmp;
	struct list_t *al;
	tmp=*ulist;
	while ((tmp)&&strcasecmp(nickname,tmp->nick))
 	{
		/* Seek through the alias list for each user. */
		for (al=tmp->alias;al&&strcasecmp(nickname,al->str);al=al->next);
		if (al) break;
		tmp=tmp->next;
	}
	return(tmp);
}

/* Function: Empties the userlist. */
void free_users(struct user_t **ulist)
{
	struct user_t *p,*tmp;
	struct list_t *s,*t;
	p=*ulist;
	while (p)
	{
		tmp=p->next;
		s=p->userhost;
		while(s)
		{
			t=s->next;
			lfree(s);
			s=t;
		}
		s=p->alias;
		while(s)
		{
			t=s->next;
			lfree(s);
			s=t;
		}
		free(p);
		p=tmp;
	}
	*ulist=NULL;
}


/* Function: Deletes a user from userlist. */
void del_user(char *nickname,struct user_t **ulist)
{
	struct user_t *p,*tmp;
	struct list_t *s,*t;
	p=*ulist;
	if ((tmp=get_user(nickname,ulist)))
	{
		if (*ulist==tmp)
		{
			(*ulist)=tmp->next;
		}
		else
		{
			while (p->next!=tmp)
			{
				p=p->next;
			}
			p->next=tmp->next;
		}
		/* Next time I'll use C++, I swear... */
		for (s=tmp->userhost;s;t=s->next,lfree(s),s=t);
		for (s=tmp->alias;s;t=s->next,lfree(s),s=t);
		free(tmp);
	}	
}
	
/* Function: Opens userlist file. */
int initusers(int ma)
{
	char *flags="r+";

	if (!(userfile=fopen(cfg.userfile,flags)))
	{
		/* Unable to open file... */
		DEBUG1("WARNING: Unable to open userfile: %s\n",cfg.userfile);
		return(0);
	}
	DEBUG1("Opening userfile: %s\n",cfg.userfile);
	return(1);
}


/* Function: Reads all users from userfile. */
int read_users(struct user_t **ulist)
{
	struct user_t *tmp=NULL;
	int ucount=0;
	char buf[256],tag[3],*val,*end;
	
	rewind(userfile);
	DEBUG0("Loading users... ");

	while(fgets(buf,255,userfile))
	{
		if (buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]='\0';
		
		/* Check for old userlist */
		if (!strncmp("NICK ",buf,5))
		{
			fprintf(stderr,"AIEE!@# This is most likely an old userlist. To change to the new format,\n");
			fprintf(stderr,"please use the userconv.pl script distributed with the JanBot source.\n");
			exit(1);
		}

		mystrncpy(tag,buf,2);
		for (val=buf+2,end=val;*end!='\0';end++);
		for (end--;*end==' ';end--);
		*(++end)='\0';

		if (!strncasecmp("NI",tag,2))
		{
			tmp=add_user(val,&users);
			ucount++;
		}
		else if (tmp&&!strncasecmp("PW",tag,2))
		{
			strncpy(tmp->passwd,val,13);
		}
		else if (tmp&&!strncasecmp("LV",tag,2))
		{
			sscanf(val,"%d",(int *)&tmp->level);
		}
		else if (tmp&&!strncasecmp("LM",tag,2))
		{
			sscanf(val,"%d",(int *)&tmp->dcclimit);
		}
		else if (tmp&&!strncasecmp("DI",tag,2))
		{
			strcpy(tmp->dir,val);
		}
		else if (tmp&&!strncasecmp("FL",tag,2))
		{
			sscanf(val,"%d",(int *)&tmp->flags);
		}
		else if (tmp&&!strncasecmp("RA",tag,2))
		{
			sscanf(val,"%d",(int *)&tmp->ratio);
		}
		else if (tmp&&!strncasecmp("UL",tag,2))
		{
			sscanf(val,"%llu",(unsigned long long*)&tmp->ul);
		}
		else if (tmp&&!strncasecmp("DL",tag,2))
		{
			sscanf(val,"%llu",(unsigned long long*)&tmp->dl);
		}
		else if (tmp&&!strncasecmp("CR",tag,2))
		{
			sscanf(val,"%llu",(unsigned long long*)&tmp->cr);
		}
		else if (tmp&&!strncasecmp("HO",tag,2))
		{
			add_userhost(tmp->nick,val,ulist);
		}
		else if (tmp&&!strncasecmp("AL",tag,2))
		{
			add_alias(tmp->nick,val,ulist);
		}
	}
	DEBUG2("%d user%s loaded.\n",ucount,ucount==1?"":"s");
	return(ucount);
}


/* Function: Calls write_users() as scheduled. */
void write_scheduled(struct user_t **ulist)
{
	static time_t t;
	static int writepid=0;
	int idx;

	if (writepid)
	{
		if (waitpid(writepid,NULL,WNOHANG)>0)
		{
			writepid=0;
		}
		return;
	}
	if (!t) t=time(NULL);
	if ((t+cfg.scheduledwrite)<=time(NULL))
	{
		if (!(writepid=fork()))
		{
			for (idx=0;idx<p_argc;idx++)
			{
				memset(p_argv[idx],0,strlen(p_argv[idx]));
			}
			sprintf(p_argv[0],"%s%s",cfg.nick,USERLIST_CHILD);
			write_users(ulist);
			_exit(0);
		}
		t=time(NULL);
	}
}

/* Function: Writes the userlist to the ascii user file. */
void write_users(struct user_t **ulist)
{
	int ucount=0,utotal=0;
#ifdef ENABLE_DEBUG
	time_t start=time(NULL);
#endif /* ENABLE_DEBUG */
	struct user_t *tmp;

	rewind(userfile);
	ftruncate(fileno(userfile),0);

	for (tmp=*ulist;tmp;tmp=tmp->next)
	{
		ucount+=write_single_user(tmp);
		utotal++;
	}
	DEBUG3("Wrote %d out of %d users to userfile in %d seconds.\n",ucount,utotal,(int)(time(NULL)-start));
}

/* Function: Writes a single user entry to file. */
int write_single_user(struct user_t *tmp)
{
	struct list_t *p;

	fprintf(userfile,"NI%s\n",tmp->nick);
	fprintf(userfile,"PW%s\n",tmp->passwd);
	fprintf(userfile,"LV%d\n",tmp->level);
	fprintf(userfile,"LM%d\n",tmp->dcclimit);
	fprintf(userfile,"DI%s\n",tmp->dir);
	fprintf(userfile,"FL%d\n",tmp->flags);
	fprintf(userfile,"RA%d\n",tmp->ratio);
	fprintf(userfile,"UL%llu\n",tmp->ul);
	fprintf(userfile,"DL%llu\n",tmp->dl);
	fprintf(userfile,"CR%llu\n",tmp->cr);
	for (p=tmp->userhost;p;p=p->next)
	{
		fprintf(userfile,"HO%s\n",p->str);
	}			
	for (p=tmp->alias;p;p=p->next)
	{
		fprintf(userfile,"AL%s\n",p->str);
	}
	return(1);
}


/* Function: Adds a user@host mask to a user. */
void add_userhost(char *nick,char *hostmask,struct user_t **ulist)
{
	struct user_t *tmp;
	struct list_t *u,*p;

	if ((tmp=get_user(nick,ulist)))
	{
		/* Check to see if the hostmask already exists. Added
		   after numerous requests from ZigZag. */
		for (u=tmp->userhost;u&&strncasecmp(u->str,hostmask,USERHOST_LEN);u=u->next);
		if (u) return; /* Already exists. */

		if (!(u=ldup(hostmask))) return;
		if (!tmp->userhost)
		{
			tmp->userhost=u;
		}
		else
		{
			for (p=tmp->userhost;p->next;p=p->next);
			p->next=u;
		}
	}
}

/* Function: Removes a userhost mask from a user. */
void del_userhost(char *nick,int num,struct user_t **ulist)
{
	struct user_t *tmp;
	struct list_t *p,*q;
	int index;	

	/* This might get nasty if we have a lot of hosts, but for now... */

	if (!(tmp=get_user(nick,ulist))) return;
	if (!(p=tmp->userhost)) return;
	if (!num)
	{
		tmp->userhost=p->next;
		lfree(p);
		return;
	}
	for (index=1,p=tmp->userhost;index<num;index++,p=p->next);
	q=p->next;
	p->next=q->next;
	lfree(q);
}

/* Function: Returns the number of registered userhost masks for a user. */
int userhost_count(struct list_t *p)
{
	if (!p) return(0);
	else return (1+userhost_count(p->next));
}

/* Function: Adds an alias to a user. */
void add_alias(char *nick,char *alias,struct user_t **ulist)
{
	struct user_t *tmp;
	struct list_t *u,*p;
	
	if ((tmp=get_user(nick,ulist)))
	{
		if (!(u=ldup(alias))) return;
		if (!tmp->alias)
		{
			tmp->alias=u;
		}
		else
		{
			for (p=tmp->alias;p->next;p=p->next);
			p->next=u;
		}
	}
}

/* Function: Removes an alias from a user. */
void del_alias(char *nick,int num,struct user_t **ulist)
{
	struct user_t *tmp;
	struct list_t *p,*q;
	int index;	

	if (!(tmp=get_user(nick,ulist))) return;
	if (!(p=tmp->alias)) return;
	if (!num)
	{
		tmp->alias=p->next;
		lfree(p);
	}
	for (index=1;index<num;index++,p=p->next);
	q=p->next;
	p->next=q->next;
	lfree(q);
}

/* Function: Returns the number of registered aliases for a user. */
int alias_count(struct list_t *p)
{
	if (!p) return(0);
	else return (1+alias_count(p->next));
}
	
/* Function: Authenticates a user by nick+userhost+passwd */
int authenticate(char *a_nick,char *a_userhost,char *a_passwd,struct user_t **ulist)
{
	struct user_t *usr;
	struct list_t *p;
	int matched=0;
	
	/* Changed from get_user() to get_alias() to support aliases. */
	if (!(usr=get_alias(a_nick,ulist)))
	{
		/* No user matching the nick */
		DEBUG1("Auth error: No such nick: %s\n",a_nick);
		return(1);
	}

	/* From the irc2.9p1 server code:
	**
	** ident is fun.. ahem
	** prefixes used:
	**	none	I line with ident
	**	^	I line with OTHER type ident
	**	~	I line, no ident
	**	+	i line with ident
	**	=	i line with OTHER type ident
	**	-	i line, no ident
	*/

	if (*a_userhost=='^') a_userhost++;
	else if (*a_userhost=='~') a_userhost++;
	else if (*a_userhost=='+') a_userhost++;
	else if (*a_userhost=='=') a_userhost++;
	else if (*a_userhost=='-') a_userhost++;
	
	DEBUG1("Checking host: %s\n",a_userhost);
	p=usr->userhost;
	while(p)
	{
		if (!fnmatch(p->str,a_userhost,FNM_NOESCAPE))
		{
			matched=1;
		}
		p=p->next;
	}
	if (!matched)
	{
		/* No userhost matching of this user. */
		DEBUG0("Auth error: No matching userhost.\n");
		return(1);
	}

	if (chkpasswd(a_nick,a_passwd,ulist))
	{
		/* Password is incorrect. */
		DEBUG0("Auth error: Password mismatch.\n");
		return(1);
	}

	DEBUG1("Auth: %s authenticated.\n",usr->nick);
	return(0);	
}


/* Function: Sets a password for a user. */
int setpasswd(char *nick,char *passwd,struct user_t **ulist)
{
	struct user_t *usr;
	static char *saltlist="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
	char newsalt[3]={0,0,0};
	char newpasswd[9];	

	if (!(usr=get_user(nick,ulist)))
	{
		/* Oops, no such user. */
		return(1);
	}

	newsalt[0]=saltlist[rand()%strlen(saltlist)];
	newsalt[1]=saltlist[rand()%strlen(saltlist)];
	
	memset(newpasswd,0,9);
	strncpy(newpasswd,passwd,8);
	
	mystrncpy(usr->passwd,crypt(newpasswd,newsalt),13);

	return(0);
}

/* Function: Validates a user's passwd */
int chkpasswd(char *nick,char *passwd,struct user_t **ulist)
{
	struct user_t *usr;
	char newsalt[3]={0,0,0};
	char newpasswd[9];	
		
	/* Changed from get_user() to get_alias() to support aliases. */
	if (!(usr=get_alias(nick,ulist)))
	{
		/* No such luck. User does not exist. */
		return(1);
	}
	
	/* Get the salt encryption code. */
	strncpy(newsalt,usr->passwd,2);

	memset(newpasswd,0,9);
	strncpy(newpasswd,passwd,8);

	if (strcmp(crypt(newpasswd,newsalt),usr->passwd))
	{
		/* Encrypted strings doesn't match. This is not the passwd. */
		return(1);
	}
	return(0);
}

int setlevel(char *nick,int lvl,struct user_t **ulist)
{
	struct user_t *tmp;
	if ((tmp=get_user(nick,ulist)))
	{
		tmp->level=lvl;
		return(0);
	}
	return(1);
}

int setlimit(char *nick,int lim,struct user_t **ulist)
{
	struct user_t *tmp;
	if ((tmp=get_user(nick,ulist)))
	{
		tmp->dcclimit=lim;
		return(0);
	}
	return(1);
}

/* Function: Does what it says, parses the USERHOST server reply. */
void parse_userhost_reply(char *args)
{
	struct chat_t *usr;
	struct user_t *tmp;
	char *n,*u,*h,pw[10];
	char nh[50]; /* I've seen some really long hostnames in my life. */

	if (!(usr=find_chat(uhquery.caller,&chatlist)))
	{
		/* We won't do anything unless the caller is connected. */
		return;
	}
	
	if (!strlen(args))
	{
		uprintf(usr,"Cannot find \002%s\002 on iRC!\n",uhquery.nick);
		return;
	}

	n=strtok(args,"=");
	if (n[strlen(n)-1]=='*') n[strlen(n)-1]='\0'; 
	u=strtok(NULL,"@")+1;
	h=strtok(NULL,"@");

	/* Strip the ident characters. Fixed to 2.9p1 compliance. */
	if (*u=='^') u++;
	else if (*u=='~') u++;
	else if (*u=='+') u++;
	else if (*u=='=') u++;
	else if (*u=='-') u++;

	strcpy(nh,u);
	strcat(nh,"@");
	strcat(nh,create_hostmask(h));

	/* This will be the temporary password. */
	strcpy(pw,n);
	for (u=pw;*u!='\0';u++) *u=tolower(*u);

	if ((tmp=get_user(n,&users)))
	{
		add_userhost(n,nh,&users);
		uprintf(usr,"Added hostmask \002%s\002 to user \002%s\002.\n",nh,tmp->nick);
	}
	else
	{
		if ((tmp=add_user(n,&users)))
		{
			add_userhost(n,nh,&users);
			setpasswd(n,pw,&users);
			uprintf(usr,"Added new user \002%s\002 with hostmask \002%s\002.\n",n,nh);
			send_to_server("NOTICE %s :Hi there! You have just been added to my userlist\n",n);
			send_to_server("NOTICE %s :with the hostmask \002%s\002. To connect you must\n",n,nh);
			send_to_server("NOTICE %s :always use your current nick, issue the command\n",n);
			send_to_server("NOTICE %s :'/CTCP %s AUTH \002%s\002', and use the chat to play.\n",n,cfg.nick,pw);
			send_to_server("NOTICE %s :\002%s\002 is your password, and it can be changed later.\n",n,pw);
		}
		else
		{
			uprintf(usr,"Unable to add new user!\n");
		}
	}	
}

/* Function: Reads userdata from keyboard, primarily to set up first user. */
void enter_new_user()
{
	struct user_t *tmp;
	int lvl;
	char *pw,buf[MSG_LEN+1];

	do
	{
		printf("Enter nickname: ");
		memset(buf,0,MSG_LEN+1);
		(void)scanf("%s",buf);
		if (strlen(buf)>9)
		{
			printf("Invalid nickname!\n");
		}
	}
	while (strlen(buf)>9);

	if ((tmp=get_user(buf,&users)))
	{
		fprintf(stderr,"\002%s\002 is already registered.\n",tmp->nick);
		return;
	}
	
	if (!(tmp=add_user(buf,&users)))
	{
		fprintf(stderr,"Unable to add user.\n");
		return;
	}
	
	printf("Enter user@host mask (you may have to use \\@): ");
	memset(buf,0,MSG_LEN+1);
	(void)scanf("%s",buf);
	add_userhost(tmp->nick,buf,&users);

	printf("Enter user level (0-3): ");
	memset(buf,0,MSG_LEN+1);
	(void)scanf("%d",&lvl);
	setlevel(tmp->nick,lvl,&users);
	
	pw=getpass("Enter password: ");
	setpasswd(tmp->nick,pw,&users);
	memset(pw,0,strlen(pw));
	write_users(&users);
}
