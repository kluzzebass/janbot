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
 * $Id: commands.c,v 1.7 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>


/* List of commands:
 * Currently the only valid levels are NORMAL_USER, POWER_USER,
 * MASTER_USER and SUPERIOR_BEING. The list should be alphabetically
 * sorted, as binary search might be added later. Note: The userlevels
 * required are entry levels only. Commands might provide more functions
 * if the userlevel is higher.
 */
struct cmd_t cmdlist[]={
{ "ADD",	cmd_add,	MASTER_USER,	"<user> [user@hostmask]" },
#ifdef ENABLE_RATIOS
{ "ADJUST",	cmd_adjust,	MASTER_USER,	"<user> UL|DL|CRED <amount>"  },
#endif
{ "ALIAS",	cmd_alias,	MASTER_USER,	"<user> <alias>" },
{ "BYE",	cmd_quit,	NORMAL_USER,	NULL },
{ "CD",		cmd_cd,		NORMAL_USER,	"[directory]" },
{ "CUTCON",	cmd_cutcon,	POWER_USER,	"<user>" },
{ "DEL",	cmd_del,	MASTER_USER,	"<user> [hostnum]" },
{ "DIR",	cmd_ls,		NORMAL_USER,	NULL },
#ifdef ENABLE_RATIOS
{ "DONATE",	cmd_donate,	NORMAL_USER,	"<user> <credits>" },
#endif
{ "EXIT",	cmd_quit,	NORMAL_USER,	NULL },
{ "GET",	cmd_send,	NORMAL_USER,	"<wildcards>" },
{ "HELP",	cmd_help,	NORMAL_USER,	"[command]" },
{ "HINT",	cmd_hint,	NORMAL_USER,	NULL },
{ "KILLDCC",	cmd_killdcc,	NORMAL_USER,	"[user] <#num/#all>" },
{ "KILLQ",	cmd_killq,	NORMAL_USER,	"[user] <#num/#all>" },
{ "LIMIT",	cmd_limit,	NORMAL_USER,	"<number>" },
{ "LS",		cmd_ls,		NORMAL_USER,	NULL },
{ "MKDIR",	cmd_mkdir,	NORMAL_USER,	"<directory>" },
{ "NEXT",	cmd_next,	NORMAL_USER,	"[number]" },
{ "PASSWD",	cmd_passwd,	NORMAL_USER,	"<password>"},
{ "QUIT",	cmd_quit,	NORMAL_USER,	NULL },
#ifdef ENABLE_RATIOS
{ "RATIO",	cmd_ratio,	MASTER_USER,	"<user> <ratio>"},
#endif
{ "REHASH",	cmd_rehash,	SUPERIOR_BEING,	NULL },
{ "RELEVEL",	cmd_relevel,	MASTER_USER,	"<user> <level>"},
{ "RETIRE",	cmd_retire,	SUPERIOR_BEING,	NULL },
{ "SEND",	cmd_send,	NORMAL_USER,	"<wildcards>"},
{ "SENDTO",	cmd_sendto,	NORMAL_USER,	"<nick> <wildcards>"},
{ "SERVER",	cmd_server,	MASTER_USER,	NULL },
{ "SET",	cmd_set,	NORMAL_USER, 	"<flag>" },
{ "SETPASS",	cmd_setpass,	MASTER_USER, 	"<user> <password>" },
{ "SHOWDCC",	cmd_showdcc,	NORMAL_USER, 	"[user]" },
{ "SHOWQ",	cmd_showq,	NORMAL_USER, 	"[user]" },
{ "STATUS",	cmd_status,	MASTER_USER, 	NULL },
#ifdef ENABLE_DEBUG
{ "TEST",	cmd_test,	SUPERIOR_BEING,	NULL},
#endif /* ENABLE_DEBUG */
{ "UNALIAS",	cmd_unalias,	MASTER_USER,	"<user> <aliasnum>" },
{ "USERS",	cmd_users,	NORMAL_USER, 	NULL },
{ "WHO",	cmd_who,	POWER_USER, 	"[user]" },
{ "WHOAMI",	cmd_whoami,	NORMAL_USER,	NULL },

{ NULL,		NULL,		-1,		NULL } /* Marks end of array */
};



/* Function: Adds a new user, or adds a hostmask to an existing user. */
int cmd_add(struct chat_t *usr,char **args)
{
	struct user_t *target;

	if (!args[0]) return(0);

	if (!isvalidnick(args[0]))
	{
		uprintf(usr,"That is not a valid nickname.\n");
		return(1);
	}

	if (args[1])
	{
		if (!(target=get_user(args[0],&users)))
		{
			if ((target=add_user(args[0],&users)))
			{
				add_userhost(args[0],args[1],&users);
				uprintf(usr,"Added new user \002%s\002 with hostmask \002%s\002.\n",target->nick,args[1]);
			}
			else
			{
				uprintf(usr,"Unable to add user \002%s\002.\n",args[0]);
			}
		}
		else
		{
			add_userhost(target->nick,args[1],&users);
			uprintf(usr,"Added hostmask \002%s\002 to user \002%s\002.\n",args[1],target->nick);
		}
	}
	else
	{
		/* This is a nasty workaround for the USERHOST call. */
		mystrncpy(uhquery.nick,args[0],NICK_LEN);
		uhquery.caller=usr->chatter;
		send_to_server("USERHOST %s\n",args[0]);
		/* Actually, this is all we do. Now we just have to wait */
		/* for a server reply to this thing. It'll be handled later. */
	}
	return(1);
}

/* Function: Adjusts Upload/Download/Credit amounts for a user. */
#ifdef ENABLE_RATIOS
int cmd_adjust(struct chat_t *usr,char **args)
{
	struct user_t *u;
	struct chat_t *c;
	unsigned long long t;

	if (!args[0]||!args[1]||!args[2])
	{
		return(0);
	}
	if (!(u=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		return(1);
	}
	if (!sscanf(args[2],"%llu",&t)) return(0);
	t*=1024;
	if (!strcasecmp(args[1],"UL")) u->ul=t;
	else if (!strcasecmp(args[1],"DL")) u->dl=t;
	else if (!strcasecmp(args[1],"CRED")) u->cr=t;
	else
	{
		uprintf(usr,"Valid types are \002UL\002, \002DL\002 and \002CRED\002.\n");
		return(1);
	}
	if ((c=find_chat(u,&chatlist)))
	{
		send_to_process(c,RATIO,"%s %llu",args[1],t);
	}
	uprintf(usr,"\002%s\002's UL/DL/CRED values are now: \002%lluK/%lluK/%lluK\002.\n",u->nick,u->ul/1024,u->dl/1024,u->cr/1024);
	return(1);
}
#endif

int cmd_alias(struct chat_t *usr,char **args)
{
	struct user_t *b,*c;

	if (!args[0]||!args[1])
	{
		return(0);
	}
	
	if (!(b=get_user(args[0],&users)))
	{
		uprintf(usr,"User \002%s\002 is not registered.\n",args[0]);
		return(1);
	}

	if ((c=get_alias(args[1],&users)))
	{
		if (strcasecmp(c->nick,args[1]))
		{
			uprintf(usr,"The alias \002%s\002 already exists for user \002%s\002.\n",args[1],c->nick);
		}
		else
		{
			uprintf(usr,"User \002%s\002 is already registered.\n",c->nick);
		}
		return(1);
	}

	add_alias(b->nick,args[1],&users);
	uprintf(usr,"Added alias \002%s\002 to user \002%s\002.\n",args[1],b->nick);
	return(1);
}


/* Function: Displays or changes a users current directory. */
int cmd_cd(struct chat_t *usr,char **args)
{
	int i,dlen;
	char path[NAME_MAX+1],cmppath[NAME_MAX+1],*p,*q;
	glob_t gbuf;
	struct stat sbuf;
	
	if (!args[0])
	{
		uprintf(usr,"Current directory: \002%s\002\n",usr->chatter->dir);
#ifdef HAVE_STATFS
		sprintf(path,"%s/%s",cfg.filedir,usr->chatter->dir);
		uprintf(usr,"Free space on device: \002%s\002\n",show_free_space(path));
#endif
		return(1);
	}

	if (!strcmp(args[0],"/"))
	{
		strcpy(usr->chatter->dir,"/");
		uprintf(usr,"New directory: \002%s\002\n",usr->chatter->dir);
#ifdef HAVE_STATFS
		sprintf(path,"%s/%s",cfg.filedir,usr->chatter->dir);
		uprintf(usr,"Free space on device: \002%s\002\n",show_free_space(path));
#endif
		return(1);
	}		

	memset(&gbuf,0,sizeof(glob_t));
	sprintf(path,"%s/%s/%s",cfg.filedir,args[0][0]=='/'?"":usr->chatter->dir,args[0]);
	
	sprintf(cmppath,"%s/",cfg.filedir);
	dlen=strlen(cmppath);
	
	if (!glob(path,0,NULL,&gbuf))
	{
		for (i=0,p=q=NULL;i<gbuf.gl_pathc;i++)
		{
			p=resolve_path(gbuf.gl_pathv[i]);
			strcat(p,"/");
			if (strncmp(cmppath,p,dlen)||stat(p,&sbuf)||!(sbuf.st_mode&S_IFDIR)) continue;
			q=p;
			break;
		}
		if (q)
		{
			strcpy(usr->chatter->dir,p+strlen(cfg.filedir));
			if (usr->chatter->dir[strlen(usr->chatter->dir)-1]=='/')
			{
				usr->chatter->dir[strlen(usr->chatter->dir)-1]='\0';
			}
			if (!*usr->chatter->dir)
			{
				strcpy(usr->chatter->dir,"/");
			}
		}
		else
		{
			strcpy(usr->chatter->dir,"/");
		}
		uprintf(usr,"New directory: \002%s\002\n",usr->chatter->dir);
#ifdef HAVE_STATFS
		sprintf(path,"%s/%s",cfg.filedir,usr->chatter->dir);
		uprintf(usr,"Free space on device: \002%s\002\n",show_free_space(path));
#endif
	}
	else uprintf(usr,"Unable to switch directory!\n");
	globfree(&gbuf);
	return(1);
}


/* Function: Terminates a users connection. */
int cmd_cutcon(struct chat_t *usr,char **args)
{
	struct chat_t *c;
	struct user_t *u;
	
	if (!args[0]) return(0);
	if ((u=get_user(args[0],&users)))
	{
		if ((c=find_chat(u,&chatlist)))
		{
			if (c==usr)
			{
				uprintf(usr,"Whatever you say, boss.\n");
			}
			else
			{
				uprintf(c,"I have been instructed to terminate this connection. Bye!\n");
				uprintf(usr,"Terminating connection to \002%s\002.\n",u->nick);
			}
			kill_chat(c,&chatlist);
		}
		else
		{
			uprintf(usr,"\002%s\002 is not connected.\n",u->nick);
		}
	}
	else
	{
		uprintf(usr,"Cannot find \002%s\002 in my records.\n",args[0]);
	}
	return(1);
}


/* Function: Deletes a user, or deletes a hostmask from user. */
int cmd_del(struct chat_t *usr,char **args)
{
	struct user_t *tmp;
	struct list_t *p;
	struct chat_t *c;
	int hn,idx;
	
	if (!args[0]) return(0);
	if (!args[1])
	{
		if (!(tmp=get_user(args[0],&users)))
		{
			uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		}
		else
		if (usr->chatter==tmp)
		{
			uprintf(usr,"You cannot delete yourself!\n");
		}
		else
		{
			if (usr->chatter->level<tmp->level)
			{
				uprintf(usr,"You cannot delete a higher ranking user!\n");
				return(1);
			}
			if ((c=find_chat(tmp,&chatlist)))
			{
				uprintf(c,"Sorry, your user has been deleted. Terminating connection.\n");
				kill_chat(c,&chatlist);
				uprintf(usr,"User \002%s\002 has been deleted, and connection terminated.\n",tmp->nick);
			}
			else
			{
				uprintf(usr,"User \002%s\002 has been deleted.\n",tmp->nick);
			}
			del_user(tmp->nick,&users);
		}
	}
	else
	{
		if (!(tmp=get_user(args[0],&users)))
		{
			uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
			return(1);
		}
		if (!sscanf(args[1],"%d",&hn)) return(0);
		if ((hn<1)||(hn>userhost_count(tmp->userhost)))
		{
			uprintf(usr,"Unable to delete hostmask number \002%d\002 from user \002%s\002.\n",hn,tmp->nick);
		}
		else
		{
			for(p=tmp->userhost,idx=1;idx<hn;p=p->next,idx++);
			uprintf(usr,"Deleted hostmask \002%s\002 from user \002%s\002.\n",p->str,tmp->nick);
			del_userhost(tmp->nick,hn-1,&users);
		}
	}
	return(1);
}


/* Function: Donates credits to other users. */
#ifdef ENABLE_RATIOS
int cmd_donate(struct chat_t *usr,char **args)
{
	struct user_t *u;
	struct chat_t *c;
	unsigned long long cred;

	if (!args[0]||!args[1]) return(0);
	if (!usr->chatter->ratio)
	{
		uprintf(usr,"Users with no ratio may not donate credits.\n");
		return(1);
	}
	if (!(u=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		return(1);
	}
	sscanf(args[0],"%llu",&cred);
	if ((cred*1024)>usr->chatter->cr)
	{
		uprintf(usr,"You do not that much credits.\n");
		return(1);
	}
	u->cr+=cred*1024;
	usr->chatter->cr-=cred*1024;
	if ((c=find_chat(u,&chatlist)))
	{
		send_to_process(c,RATIO,"CRED %llu",u->cr);
	}
	uprintf(usr,"\002%lluK\002 credits has been donated to \002%s\002.\n",cred,u->nick);
	return(1);
}
#endif

/* Function: Displays help about commands. */
int cmd_help(struct chat_t *usr,char **args)
{
	int idx;
	
	if (!args[0])
	{
		showhelp(usr,"HELP");
		return(1);
	}
	
	for (idx=0;(cmdlist[idx].name)&&(strcasecmp(cmdlist[idx].name,args[0]));idx++);
	if ((cmdlist[idx].name)&&(cmdlist[idx].level<=usr->chatter->level))
	{
		showhelp(usr,cmdlist[idx].name);
	}
	else
	{
		uprintf(usr,"No such command: \002%s\002\n",args[0]);
	}
	return(1);
}

/* Function: Displays short command list. */
int cmd_hint(struct chat_t *usr,char **args)
{
	int idx,cnt;
	char cmd[20],buf[MSG_LEN+1];
	
	memset(buf,0,sizeof(buf));
	uprintf(usr,"Short command index for userlevel \002%d\002:\n",usr->chatter->level);
	for (idx=0,cnt=0;cmdlist[idx].name;idx++)
	{
		if ((usr->chatter->level)>=cmdlist[idx].level)
		{
			sprintf(cmd,"\002%-12s\002",cmdlist[idx].name);
			strcat(buf,cmd);
			if ((cnt++%4)==3)
			{
				strcat(buf,"\n");
				uprintf(usr,"%s",buf);
				*buf='\0';
			}
		}
	}
	uprintf(usr,"%s%s",buf,cnt%4?"\n":"");
	uprintf(usr,"End of list.\n");
	return(1);
}



/* Function: Remove an Active or Waiting DCC from user. */
int cmd_killdcc(struct chat_t *usr,char **args)
{
	struct user_t *tmpusr;
	struct chat_t *tmpchat;

	if (!args[0]) return(0);
	if (usr->chatter->level<POWER_USER)
	{
		if (*args[0]!='#') return(0);
		send_to_process(usr,KILL,"%s %s",usr->chatter->nick,args[0]);
	}
	else
	{
		if (!args[1])
		{
			if (*args[0]!='#') return(0);
			send_to_process(usr,KILL,"%s %s",usr->chatter->nick,args[0]);
		}
		else
		{
			if (*args[1]!='#') return(0);
			if (!(tmpusr=get_user(args[0],&users)))
			{
				uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
			}
			else
			{
				if ((tmpchat=find_chat(tmpusr,&chatlist)))
				{
					send_to_process(tmpchat,KILL,"%s %s",usr->chatter->nick,args[1]);
				}
				else
				{
					uprintf(usr,"User \002%s\002 is not connected.\n",tmpusr->nick);
				}
			}
		}
	}
	return(1);
}




/* Function: Remove a queued DCC from user. */
int cmd_killq(struct chat_t *usr,char **args)
{
	struct user_t *tmpusr;
	struct chat_t *tmpchat;

	if (!args[0]) return(0);
	if (usr->chatter->level<POWER_USER)
	{
		if (*args[0]!='#') return(0);
		send_to_process(usr,QKILL,"%s %s",usr->chatter->nick,args[0]);
	}
	else
	{
		if (!args[2])
		{
			if (*args[0]!='#') return(0);
			send_to_process(usr,QKILL,"%s %s",usr->chatter->nick,args[0]);
		}
		else
		{
			if (*args[1]!='#') return(0);
			if (!(tmpusr=get_user(args[0],&users)))
			{
				uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
			}
			else
			{
				if ((tmpchat=find_chat(tmpusr,&chatlist)))
				{
					send_to_process(tmpchat,QKILL,"%s %s",usr->chatter->nick,args[1]);
				}
				else
				{
					uprintf(usr,"User \002%s\002 is not connected.\n",tmpusr->nick);
				}
			}
		}
	}
	return(1);
}


/* Function: Sets users DCC limit. */
int cmd_limit(struct chat_t *usr,char **args)
{
	int lim;
	
	if (!args[0]||!sscanf(args[0],"%d",&lim)) return(0);
	if (lim<1)
	{
		uprintf(usr,"A DCC limit of \002%d\002 wouldn't be smart.\n",lim);
		return(1);
	}
	if (setlimit(usr->chatter->nick,lim,&users))
	{
		uprintf(usr,"Unable to set DCC limit.\n");
		return(1);
	}
	send_to_process(usr,NUM,"%d",lim);
	uprintf(usr,"Your DCC limit has been set to \002%d\002.\n",lim);
	return(1);
}

/* Function: Displays long listing of current or specified directories. */
int cmd_ls(struct chat_t *usr,char **args)
{
	int i,j,k,l,dlen;
	char **av,*empty[]={".",NULL},*p;
	char path[NAME_MAX+1],cmppath[NAME_MAX+1];
	struct dirlist_t **fp,**dp,**ap,*q;
	glob_t gbuf;
	
	/* If nothing is specified, the correct way to handle it is by listing "." */
	if (!args[0]) av=empty;
	else av=args;

	memset(&gbuf,0,sizeof(glob_t));
	for (i=0;av[i];i++)
	{
		sprintf(path,"%s/%s/%s",cfg.filedir,av[i][0]=='/'?"":usr->chatter->dir,av[i]);
		glob(path,(i>0?GLOB_APPEND:0)|GLOB_MARK,NULL,&gbuf);

	}

	if (!gbuf.gl_pathc)
	{
		uprintf(usr,"No files found.\n");
		globfree(&gbuf);
		return(1);
	}
	/* Just to be safe, we make both lists big enough. */
	fp=(struct dirlist_t **)malloc(sizeof(struct dirlist_t) * (gbuf.gl_pathc+1));
	dp=(struct dirlist_t **)malloc(sizeof(struct dirlist_t) * (gbuf.gl_pathc+1));

	sprintf(cmppath,"%s/",cfg.filedir);
	dlen=strlen(cmppath);

	for(i=j=k=0;i<gbuf.gl_pathc;i++)
	{
		p=resolve_path(gbuf.gl_pathv[i]);
		if (strncmp(cmppath,p,dlen)) continue;
		if (p[strlen(p)-1]!='/')
		{
			fp[j++]=strfdup(p);
		}
		else
		{
			p[strlen(p)-1]='\0';
			dp[k++]=strfdup(p);
		}

	}
	fp[j]=NULL;
	dp[k]=NULL;
	
	globfree(&gbuf);

	/*
	 * Sort the files and directories to make all occurrences of an item appear
	 * next to eachother. Since we use the dirlist_t structure, we may also sort
	 * on dates and sizes.
	 */

	if (j>1)
	{
		for (j=0;fp[j+1];j++)
		{
			for (l=j+1;fp[l];l++)
			{
				if (dlcmp(fp[l],fp[j],0)<0)
				{
					q=fp[j];
					fp[j]=fp[l];
					fp[l]=q;
				}
			}
		}
	}

	if (k>1)
	{
		for (k=0;dp[k+1];k++)
		{
			for (l=k+1;dp[l];l++)
			{
				if (dlcmp(dp[l],dp[k],0)<0)
				{
					q=dp[k];
					dp[k]=dp[l];
					dp[l]=q;
				}
			}
		}
	}

	/* First print all the files. */
	for (i=0;fp[i];i++) uprintf(usr,"%s\n",print_file_stats(fp[i],fp[i]->name+dlen));

	/* Then print the contents of each directory. */
	for (i=0;dp[i];i++)
	{
		sprintf(path,"%s/*",dp[i]->name);
		glob(path,0,NULL,&gbuf);
		
		if (strlen(dp[i]->name)>dlen) uprintf(usr,"%s\002%s\002:\n",i||fp[0]?"\n":"",dp[i]->name+dlen);

		if (!gbuf.gl_pathc)
		{
			globfree(&gbuf);
			continue;
		}
		
		ap=(struct dirlist_t **)malloc(sizeof(struct dirlist *) * (gbuf.gl_pathc+1));
		
		for (j=0;j<gbuf.gl_pathc;j++)
		{
			ap[j]=strfdup(gbuf.gl_pathv[j]);
		}
		ap[j]=NULL;

		if (j>1)
		{
			for (j=0;ap[j+1];j++)
			{
				for (l=j+1;ap[l];l++)
				{
					if (dlcmp(ap[l],ap[j],0)<0)
					{
						q=ap[j];
						ap[j]=ap[l];
						ap[l]=q;
					}
				}
			}
		}

		for (j=0;j<gbuf.gl_pathc;j++)
		{
			uprintf(usr,"%s\n",print_file_stats(ap[j],ap[j]->name+strlen(dp[i]->name)+1));
			free(ap[j]);
		}

		free(ap);
		globfree(&gbuf);
	}

	for (i=0;fp[i];i++) free(fp[i]);
	for (i=0;dp[i];i++) free(dp[i]);

	free(fp);
	free(dp);

	return(1);
}	

	
/* Function: Creates a new directory. */
int cmd_mkdir(struct chat_t *usr,char **args)
{
	char newdir[256];
	
	if (!args[0]) return(0);

	strcpy(newdir,cfg.filedir);
	strcat(newdir,usr->chatter->dir);
	strcat(newdir,"/");
	strcat(newdir,args[0]);
	if (mkdir(newdir,0777))
	{
		uprintf(usr,"Unable to create directory!\n");
	}
	else
	{
		uprintf(usr,"New directory created.\n");
	}
	return(1);
}

/* Function: Makes the robot send more files from the DCC queue. */
int cmd_next(struct chat_t *usr,char **args)
{
	char *t=NULL;
	int count;
	
	if (usr->chatter->flags&F_AUTO)
	{
		uprintf(usr,"This command is disabled when \002AutoSend\002 is enabled.\n");
		return(1);
	}
	if (!args[0]) count=1;
	else if ((count=atoi(t))<1) return(0);
	send_to_process(usr,NEXT,"%d",count);
	return(1);
}

/* Function: Changes a users password. */
int cmd_passwd(struct chat_t *usr,char **args)
{
	if (!args[0]) return(0);
	setpasswd(usr->chatter->nick,args[0],&users);
	uprintf(usr,"Password has been changed.\n");
	return(1);
}

/* Function: Terminate the connection, log off, quit, whatever. */
int cmd_quit(struct chat_t *usr,char **args)
{
	uprintf(usr,"Terminating connection.\n");
	usr->terminate=1;
	return(1);
}

/* Function: Changes someone's Upload/Download ratio. */
#ifdef ENABLE_RATIOS
int cmd_ratio(struct chat_t *usr,char **args)
{
	struct user_t *tmp;
	struct chat_t *c;
	int rat;
	
	if (!args[0]||!args[1]||!sscanf(args[1],"%d",&rat)) return(0);
	if (!(tmp=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find \002%s\002 in my records.\n",args[0]);
		return(1);
	}
	tmp->ratio=rat;
	if ((c=find_chat(tmp,&chatlist))) send_to_process(c,RATIO,"RATIO %d",tmp->ratio);
	if (rat) uprintf(usr,"User \002%s\002 now has an Upload/Download ratio of 1:\002%d\002.\n",tmp->nick,tmp->ratio);
	else uprintf(usr,"User \002%s\002 now has no Upload/Download ratio.\n",tmp->nick);
	return(1);
}
#endif /* ENABLE_RATIOS */

/* Function: Rereads the config file. */
int cmd_rehash(struct chat_t *usr,char **args)
{

	if (args[0]) return(0);
	uprintf(usr,"Rehashing config file...\n");
	init_config(NULL,usr);
	uprintf(usr,"Done rehashing.\n");
	return(1);
}

/* Function: Changes someone's user level. */
int cmd_relevel(struct chat_t *usr,char **args)
{
	struct user_t *tmp;
	int lvl;
	
	if (!args[0]||!args[1]) return(0);
	if (!sscanf(args[1],"%d",&lvl)) return(0);
	if (!(tmp=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find \002%s\002 in my records.\n",args[0]);
	}
	else if (usr->chatter==tmp)
	{
		uprintf(usr,"You cannot relevel yourself!\n");
	}
	else if ((lvl>SUPERIOR_BEING)||(lvl<NORMAL_USER))
	{
		uprintf(usr,"Level \002%d\002 is not a valid user level.\n",lvl);
	}
	else if (usr->chatter->level<tmp->level)
	{
		uprintf(usr,"You cannot relevel higher ranking user.\n");
	}
	else if (usr->chatter->level<lvl)
	{
		uprintf(usr,"You cannot promote another user beyond your own level.\n");
	}
	else if (setlevel(tmp->nick,lvl,&users))
	{
		uprintf(usr,"Unable to relevel user \002%s\002.\n",tmp->nick);
	}
	else {
		uprintf(usr,"User \002%s\002 has been releveled to \002%d\002.\n",tmp->nick,tmp->level);
	}
	return(1);
}


/* Function: Make the bot quit. */
int cmd_retire(struct chat_t *usr,char **args)
{
	struct chat_t *tmp;
	char boss[NICK_LEN+1];

	strcpy(boss,usr->chatter->nick);
	for (tmp=chatlist;tmp;tmp=tmp->next)
	{
		if (usr==tmp)
		{
			uprintf(tmp,"Okay, committing suicide.\n");
		}
		else
		{
			uprintf(tmp,"I have been instructed by \002%s\002 to retire. Bye!\n",boss);
		}
		kill_chat(tmp,&chatlist);
	}
	DEBUG1("Shutdown initiated by %s.\n",boss);
	write_users(&users);
	free_users(&users);

	/* BOOM */
	log(ALL,"Exiting on request from %s.",boss);
	log(ALL,"***  LOG ENDED  ***");
	DEBUG0("Terminating.\n");
	exit(0);
}

/* Function: Sends all files matching filemask. */
int cmd_send(struct chat_t *usr,char **args)
{
	int i,j;
	char **na,*ni;


	for (i=0;args[i];i++); /* Count the number of arguments. */

	if (!(na=(char **)malloc(sizeof(char *)*(i+2)))||!(ni=strdup(usr->nick)))
	{
		if (na) free(na);
		uprintf(usr,"Doh! Unable to allocate more memory.\n");
		return(1);
	}
	
	memset(na,0,sizeof(char *)*(i+2));
	na[0]=ni;
	for(j=0;j<i;j++) na[j+1]=args[j];
	i=cmd_sendto(usr,na); /* Call the SENDTO function that does all the work. */
	free(ni);
	free(na);
	return(i);
}
	

/* Function: Sends all files matching filemask to the nick given as the first argument */
int cmd_sendto(struct chat_t *usr,char **args)
{
	int i,j=0,dlen;
	char *p,path[NAME_MAX+1],cmppath[NAME_MAX+1],*efile=NULL;
	struct file_t *l,*m;
	glob_t gbuf;
	struct stat sbuf;
	
	if (!args[1]) return(0);

	memset(&gbuf,0,sizeof(glob_t));
	for (i=1;args[i];i++)
	{
		sprintf(path,"%s/%s/%s",cfg.filedir,args[i][0]=='/'?"":usr->chatter->dir,args[i]);
		glob(path,i>1?GLOB_APPEND:0,NULL,&gbuf);
	}

	sprintf(cmppath,"%s/",cfg.filedir);
	dlen=strlen(cmppath);

	if (gbuf.gl_pathc) for (i=0;i<gbuf.gl_pathc;i++)
	{
		p=resolve_path(gbuf.gl_pathv[i]);
		if (strncmp(cmppath,p,dlen)||stat(p,&sbuf)) continue;

		if (sbuf.st_mode&S_IFREG)
		{
			if ((efile=enquote(p)))
			{
				send_to_process(usr,SEND,"%s %s",args[0],efile);
				free(efile);
			}
			else uprintf(usr,"Unable to send file \002%s\002: Internal error.\n",base_name(p));
			j++;
		}
		else if (sbuf.st_mode&S_IFDIR)
		{
#if ENABLE_TAR
			if (usr->chatter->flags&F_TAR)
			{
				if ((efile=enquote(p)))
				{
					send_to_process(usr,TAR,"%s %s",args[0],efile);
					free(efile);
				}
				else uprintf(usr,"Unable to send file \002%s.tar\002: Internal error.\n",base_name(p));
				j++;
			}
			else
#endif
			if ((l=create_reclist(p)))
			{
				for (m=l;l;m=l,l=l->next,free(m))
				{
					if ((efile=enquote(l->name)))
					{
						send_to_process(usr,SEND,"%s %s",args[0],efile);
						free(efile);
					}
					else uprintf(usr,"Unable to send file \002%s\002: Internal error.\n",base_name(l->name));
					j++;
				}
			}
		}
	}
	
	if (!j)
	{
		uprintf(usr,"No matching files or directories.\n");
	}

	globfree(&gbuf);
	return(1);
}

/* Function: List/Change/Add/Remove servers. */
int cmd_server(struct chat_t *usr,char **args)
{
	struct server_t *tmp,*tmp2;
	int idx=0;
	
	if (!args[0])
	{
		uprintf(usr,"Server list:\n");
		for (idx=1,tmp=cfg.servlist;tmp;tmp=tmp->next,idx++)
		{
			if (tmp==current_server)
			{
				uprintf(usr,"%2d) %s %d (Current)\n",idx,tmp->name,tmp->port);
			}
			else
			{
				uprintf(usr,"%2d) %s %d\n",idx,tmp->name,tmp->port);
			}
		}
		uprintf(usr,"End of list.\n");
	}
	else if (*args[0]=='-')
	{
		if ((idx=atoi(args[0]+1)))
		{
			/* Delete server number */
			uprintf(usr,del_server(idx));
			return(1);
		}
		else
		{
			uprintf(usr,"Sorry, \002%s\002 is not a valid server number.\n",args[0]);
			return(1);
		}
	}
	else if (!strcmp(args[0],"0")||(idx=atoi(args[0])))
	{
		/* Switch to this server */
		if (idx==1)
		{
			uprintf(usr,"Switching to server \002%s:%d\002.\n",cfg.servlist->name,cfg.servlist->port);
			current_server=NULL;
			close(sconn.servfd);
		}
		else
		{
			for (--idx,tmp=cfg.servlist;(tmp->next)&&(idx>0);tmp=tmp->next,--idx);
			if (!idx)
			{
				for (tmp2=cfg.servlist;(tmp2->next)!=tmp;tmp2=tmp2->next);
				uprintf(usr,"Switching to server \002%s:%d\002.\n",tmp->name,tmp->port);
				current_server=tmp2;
				close(sconn.servfd);
			}
			else
			{
				uprintf(usr,"Unable to switch server.\n");
			}
		}
	}
	else
	{
		/* Add this server */
		if (args[1])
		{
			/* Use this port. */
			if ((idx=atoi(args[1]))&&(idx<65536))
			{
				add_server(args[0],idx);
				uprintf(usr,"Added server \002%s:%d\002 to server list.\n",args[0],idx);
			}
			else
			{
				uprintf(usr,"Invalid port.\n");
			}
		}
		else
		{
			/* Use default port. */
			add_server(args[0],PORT);
			uprintf(usr,"Added server \002%s:%d\002 to server list.\n",args[0],PORT);
		}
	}
	return(1);
}

/* Function: Displays or sets user options. */
int cmd_set(struct chat_t *usr,char **args)
{
	struct user_t *u=usr->chatter;
	
	if (!args[0])
	{
		uprintf(usr,"Settings for user \002%s\002:\n",u->nick);

		uprintf(usr,"  Autoactivate DCCs  [\002AUTO\002]: \002%s\002\n",(u->flags&F_AUTO)?"On":"Off");
		uprintf(usr,"  Sorting method     [\002SORT\002]: \002%s\002\n",(u->flags&F_SORT)?"Date":"Name");
		uprintf(usr,"  Message of the day [\002MOTD\002]: \002%s\002\n",(u->flags&F_MOTD)?"Shown":"Hidden");
		uprintf(usr,"  Autorequest RESUME [\002RSUM\002]: \002%s\002\n",(u->flags&F_RSUM)?"On":"Off");
#if ENABLE_TAR
		uprintf(usr,"  Autosend TAR files [\002 TAR\002]: \002%s\002\n",(u->flags&F_TAR)?"On":"Off");
#endif
		uprintf(usr,"End of settings.\n");		
	}
	else if (!strcasecmp(args[0],"AUTO"))
	{
		u->flags^=F_AUTO;
		uprintf(usr,"Automatic DCC activation is now %s.\n",(u->flags&F_AUTO)?"enabled":"disabled");
		send_to_process(usr,AUTO,"%d %d",u->flags&F_AUTO?1:0,u->flags&F_RSUM?1:0);
	}
	else if (!strcasecmp(args[0],"SORT"))
	{
		u->flags^=F_SORT;
		uprintf(usr,"Direcory listings are now sorted by %s.\n",(u->flags&F_SORT)?"date":"name");
	}
	else if (!strcasecmp(args[0],"MOTD"))
	{
		u->flags^=F_MOTD;
		uprintf(usr,"Message of the day is now %s.\n",(u->flags&F_MOTD)?"shown":"hidden");
	}
	else if (!strcasecmp(args[0],"RSUM"))
	{
		u->flags^=F_RSUM;
		uprintf(usr,"Automatic DCC RESUME request is now %s.\n",(u->flags&F_RSUM)?"enabled":"disabled");
		send_to_process(usr,AUTO,"%d %d",u->flags&F_AUTO?1:0,u->flags&F_RSUM?1:0);
	}
#if ENABLE_TAR
	else if (!strcasecmp(args[0],"TAR"))
	{
		u->flags^=F_TAR;
		uprintf(usr,"Autosending TAR files is now %s.\n",(u->flags&F_TAR)?"enabled":"disabled");
	}
#endif
	else
	{
#if ENABLE_TAR
		uprintf(usr,"Available flags: \002AUTO SORT MOTD RSUM TAR\002.\n");
#else
		uprintf(usr,"Available flags: \002AUTO SORT MOTD RSUM\002.\n");
#endif
	}
	return(1);
}

/* Function: Set another user's password. */
int cmd_setpass(struct chat_t *usr,char **args)
{
	struct user_t *tmp;

	if (!args[0]||!args[1]) return(0);
	if (!(tmp=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		return(1);
	}
	setpasswd(args[0],args[1],&users);
	uprintf(usr,"Password for user \002%s\002 has been changed.\n",tmp->nick);
	return(1);
}

/* Function: Display DCC information for a user. */
int cmd_showdcc(struct chat_t *usr,char **args)
{
	struct user_t *tmpusr;
	struct chat_t *tmpchat;
	
	if (!args[0])
	{
		send_to_process(usr,LIST,"%s",usr->chatter->nick);
		return(1);
	}
	if (!(tmpusr=get_user(args[0],&users)))
	{
		uprintf(usr,"Sorry, can't find user \002%s\002 in my records.\n",args[0]);
		return(1);
	}
	if ((tmpusr!=usr->chatter)&&(usr->chatter->level<POWER_USER))
	{
		uprintf(usr,"With your current userlevel, that is impossible.\n");
	}
	else
	{
		if ((tmpchat=find_chat(tmpusr,&chatlist)))
		{
			send_to_process(tmpchat,LIST,"%s",usr->chatter->nick);
		}
		else
		{
			uprintf(usr,"Sorry, \002%s\002 is not connected.\n",tmpusr->nick);
		}
	}
	return(1);
}

/* Function: Display DCC Queue information for a user. */
int cmd_showq(struct chat_t *usr,char **args)
{
	struct user_t *tmpusr;
	struct chat_t *tmpchat;
	
	if (!args[0])
	{
		send_to_process(usr,QLIST,"%s",usr->chatter->nick);
		return(1);
	}
	if (!(tmpusr=get_user(args[0],&users)))
	{
		uprintf(usr,"Sorry, can't find user \002%s\002 in my records.\n",args[0]);
	}
	if ((tmpusr!=usr->chatter)&&(usr->chatter->level<POWER_USER))
	{
		uprintf(usr,"With your current userlevel, that is impossible.\n");
		return(1);
	}
	else
	{
		if ((tmpchat=find_chat(tmpusr,&chatlist)))
		{
			send_to_process(tmpchat,QLIST,"%s",usr->chatter->nick);
		}
		else
		{
			uprintf(usr,"Sorry, \002%s\002 is not connected.\n",tmpusr->nick);
		}
	}
	return(1);
}

/* Function: Displays some status information about the robot. */
int cmd_status(struct chat_t *usr,char **args)
{
	int cnt,cnt2;
	struct user_t *utmp;
	struct chat_t *ctmp;
#ifdef HAVE_SYS_UTSNAME_H
	struct utsname ub;

	memset(&ub,0,sizeof(struct utsname));
#endif

	uprintf(usr,"Status information for \002%s\002:\n",cfg.nick);
	uprintf(usr,"  Version:          \002%s\002\n",BOTVERSION);
#ifdef HAVE_SYS_UTSNAME_H
	if (!uname(&ub))
		uprintf(usr,"  System:           \002%s %s\002\n",ub.sysname,ub.release);
	else
		uprintf(usr,"  System:           \002Unknown.\002\n");
#endif
	uprintf(usr,"  Uptime:           \002%s\002\n",convert_uptime(time(NULL)-bot_uptime));
	uprintf(usr,"  Idletime:         \002%s\002\n",convert_idle(time(NULL)-idle_time));
	for (cnt=0,utmp=users;utmp;utmp=utmp->next,cnt++);
	uprintf(usr,"  Registered users: \002%d\002\n",cnt);
	for (cnt=cnt2=0,ctmp=chatlist;ctmp;ctmp=ctmp->next,cnt++) cnt2+=ctmp->filenum;
	uprintf(usr,"  Connected users:  \002%d\002\n",cnt);
	uprintf(usr,"  Total open DCCs:  \002%d\002\n",cnt2);
	if (current_server) uprintf(usr,"  Current server:   \002%s:%d\002\n",current_server->name,current_server->port);
	uprintf(usr,"  Flags:            \002%s%s%s\002\n",cfg.reconnect?"Reconnect ":"",cfg.overwrite?"Overwrite ":"",cfg.walking?"ServerWalking ":"");
	uprintf(usr,"  DCC Timeout:      \002%d\002 min.\n",cfg.dcctimeout/60);
	uprintf(usr,"  Chat Timeout:     \002%d\002 min.\n",cfg.chattimeout/60);
	uprintf(usr,"  Def. DCC limit:   \002%d\002\n",cfg.dcclimit);
	uprintf(usr,"  DCC Block Size:   \002%d\002\n",cfg.dccblocksize);
#ifdef ENABLE_RATIOS
	uprintf(usr,"  Def. UL/DL Ratio: \002%d\002\n",cfg.defaultratio);
#endif
	uprintf(usr,"End of report.\n");
	return(1);
}

/* Function: Test function for debugging/development purposes. */
#ifdef ENABLE_DEBUG
int cmd_test(struct chat_t *usr,char **args)
{
	struct file_t *p,*q;
	char dir[NAME_MAX+1];

	strcpy(dir,cfg.filedir);
	if (!(p=create_reclist(dir)))
	{
		uprintf(usr,"No files found.\n");
		return(1);
	}
	for (q=p;q;q=q->next)
	{
		uprintf(usr,"%lu %lu \002%s\002\n",q->date,q->size,q->name+strlen(cfg.filedir));
	}
	while (p)
	{
		q=p->next;
		free(p);
		p=q;
	}
	return(1);
}
#endif /* ENABLE_DEBUG */

/* Function: Removes an alias from a user. */
int cmd_unalias(struct chat_t *usr,char **args)
{
	struct list_t *p,*q;
	struct user_t *u;
	int idx,i;

	if (!args[0]||!args[1]||(idx=atoi(args[1]))<1) return(0);

	if (!(u=get_user(args[0],&users)))
	{
		uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		return(1);
	}

	if (!u->alias)
	{
			uprintf(usr,"User \002%s\002 has no aliases set.\n",u->nick);
			return(1);
	}

	if (idx==1)
	{
		p=u->alias;
		u->alias=p->next;
	}
	else
	{
		for (p=u->alias,i=1;p&&i<idx;p=p->next,i++);
		if (!p)
		{
			uprintf(usr,"Invalid alias reference for user \002%s\002\n",u->nick);
			return(1);
		}
		for (q=u->alias;q->next!=p;q=q->next);
		q->next=p->next;
	}
	uprintf(usr,"Alias \002%s\002 has been removed from user \002%s\002.\n",p->str,u->nick);
	lfree(p);
	return(1);
}

/* Function: Displays brief listing of users and their level. */
int cmd_users(struct chat_t *usr,char **args)
{
	struct user_t *p;
	int cnt;
	char str[30],buf[MSG_LEN+1];
	
	*buf='\0';
	uprintf(usr,"List of registered users:\n");
	for (p=users,cnt=0;p;p=p->next)
	{
		sprintf(str,"\002%-9s\002 (\002%1d\002)   ",p->nick,p->level);
		strcat(buf,str);
		if ((cnt++%3)==2)
		{
			uprintf(usr,"%s\n",buf);
			*buf='\0';
		}
	}
	uprintf(usr,"%s%s",buf,cnt%3?"\n":"");
	uprintf(usr,"End of list.\n");
	return(1);
}

/* Function: Lists connected user, or info on specific user. */
int cmd_who(struct chat_t *usr,char **args)
{
	struct chat_t *tmp;
	char s[NICK_LEN*2+4];
	struct user_t *u;
	struct chat_t *c;
	struct list_t *p;
	int index;
	
	if (!args[0])
	{
		uprintf(usr,"User                  Lvl  Idle      DCCs  Q'ed\n");
		for(tmp=chatlist;tmp;tmp=tmp->next)
		{
			if (strcasecmp(tmp->nick,tmp->chatter->nick))
			{
				sprintf(s,"\002%s\002 (%s)",tmp->chatter->nick,tmp->nick);
			}
			else
			{
				sprintf(s,"\002%s\002",tmp->chatter->nick);
			}
			uprintf(usr,"%-22s %3d  %s  %4d  %4d\n",s,tmp->chatter->level,convert_idle(time(NULL)-(tmp->itime)),tmp->filenum,tmp->queued);
		}
		uprintf(usr,"End of list.\n");
	}
	else
	{
		/* Changed from get_user() to get_alias() to support aliases. */
		if ((u=get_alias(args[0],&users)))
		{
			uprintf(usr,"User information for: \002%s\002 ",u->nick);
			if ((c=find_chat(u,&chatlist))&&strcasecmp(c->nick,u->nick))
			{
				uprintf(usr,"(Currently connected as \002%s\002)",c->nick);
			}
			uprintf(usr,"\n  Directory:  \002%s\002\n",u->dir);
			uprintf(usr,"  User level: \002%d\002 ",u->level);
			if (u->level==NORMAL_USER) uprintf(usr,"(Normal User)\n");
			else if (u->level==POWER_USER) uprintf(usr,"(Power User)\n");
			else if (u->level==MASTER_USER) uprintf(usr,"(Master User)\n");
			else if (u->level==SUPERIOR_BEING) uprintf(usr,"(Superior Being)\n");
			else uprintf(usr,"(Unknown)\n");
			for (index=0,p=u->alias;p;index++,p=p->next)
			{
				if (!index)
				{
					uprintf(usr,"    Aliases:  %.2d: \002%s\002\n",index+1,p->str);
				}
				else
				{
					uprintf(usr,"              %.2d: \002%s\002\n",index+1,p->str);
				}
			}
			for (index=0,p=u->userhost;p;index++,p=p->next)
			{
				if (!index)
				{
					uprintf(usr,"  Hostmasks:  %.2d: \002%s\002\n",index+1,p->str);
				}
				else
				{
					uprintf(usr,"              %.2d: \002%s\002\n",index+1,p->str);
				}
			}

			uprintf(usr,"  DCC limit:  \002%d\002\n",u->dcclimit);

			uprintf(usr,"  Flags:      \002%s%s%s%s\002\n",(u->flags&F_AUTO)?"AutoSend ":"",
				(u->flags&F_SORT)?"SortByDate ":"SortByName ",(u->flags&F_MOTD)?"ShowMOTD ":"",
				(u->flags&F_RSUM)?"AutoResume ":"");

#ifdef ENABLE_RATIOS
			if (u->ratio) uprintf(usr,"  Ratio:      1:\002%d\002\n",u->ratio);
			else uprintf(usr,"  Ratio:      \002No ratio.\002\n");
			uprintf(usr,"  UL/DL/Cred: \002%lluK/%lluK/%lluK\002\n",(u->ul)/1024,(u->dl)/1024,(u->cr)/1024);
#endif
			if (c)
			{
				uprintf(usr,"  Idletime:   \002%s\002\n",convert_idle(time(NULL)-(c->itime)));
				uprintf(usr,"  Open DCCs:  \002%d\002\n",c->filenum);
				uprintf(usr,"  Q'ed DCCs:  \002%d\002\n",c->queued);				
			}
			else
			{
				uprintf(usr,"  \002%s\002 is not connected.\n",u->nick);
			}
			uprintf(usr,"End of list.\n");
		}
		else
		{
			uprintf(usr,"Cannot find user \002%s\002 in my records.\n",args[0]);
		}
	}
	return(1);
}
		
/* Function: Lists information about user. */
int cmd_whoami(struct chat_t *usr,char **args)
{
	char *av[2]={usr->chatter->nick,NULL};
	/* Shortcut of the day. */
	cmd_who(usr,av);
	return(1);
}
