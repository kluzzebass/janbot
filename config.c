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
 * $Id: config.c,v 1.6 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>

/* The configuration container. */
struct cfg_t cfg;


struct cfgfunc_t cfgfunc[]={
{ "Nick",			cfg_nick			},
{ "UserInfo",			cfg_userinfo			},
{ "Server",			cfg_server			},
{ "BotHome",			cfg_bothome			},
{ "FileDir",			cfg_filedir			},
{ "UserFile",			cfg_userfile			},
{ "StealthComment",		cfg_stealthcomment		},
{ "StealthVersion",		cfg_stealthversion		},
{ "LogFile",			cfg_logfile			},
{ "HelpFile",			cfg_helpfile			},
{ "MotdFile",			cfg_motdfile			},
{ "Reconnect",			cfg_reconnect			},
{ "ServerWalking",		cfg_serverwalking		},
{ "Overwrite",			cfg_overwrite			},
{ "TightArsedSecurity",	cfg_tightarsedsecurity	},
{ "StealthMode",		cfg_stealthmode			},
{ "PublicAccess",		cfg_publicaccess		},
{ "LogLevel",			cfg_loglevel			},
{ "DCCTimeout",			cfg_dcctimeout			},
{ "ChatTimeout",		cfg_chattimeout			},
{ "PingTimeout",		cfg_pingtimeout			},
{ "DCCLimit",			cfg_dcclimit			},
{ "DCCBlockSize",		cfg_dccblocksize		},
{ "ScheduledWrite",		cfg_scheduledwrite		},
{ "DefaultFlags",		cfg_defaultflags		},
#ifdef ENABLE_RATIOS
{ "DefaultRatio",		cfg_defaultratio		},
#else
{ "DefaultRatio",		NULL				},
#endif /* ENABLE_RATIOS */
{NULL,				NULL				}
};


/* Function: Initializes the config structure with default values. */
void init_config(char *cfgfile,struct chat_t *u)
{
	static int rehashed=0;

	/* Set up the default values. */

	/* This is what we professionals refer to as a "kludge"... Oh well. */
	if (!rehashed)
	{
		if (cfgfile)
		{
			strcpy(cfg.configfile,cfgfile);
			purify_path(cfg.configfile);
		}
		else
		{
			strcpy(cfg.configfile,gethomepath());
			strcat(cfg.configfile,"/"CONFIGFILE);
		}
	}

#ifdef RECONNECT
	cfg.reconnect=1;
#else
	cfg.reconnect=0;
#endif
#ifdef SERVER_WALKING
	cfg.walking=1;
#else
	cfg.walking=0;
#endif
#ifdef OVERWRITE
	cfg.overwrite=1;
#else
	cfg.overwrite=0;
#endif
	cfg.loglevel=ALL;

#ifdef PUBLIC_ACCESS
	cfg.publicaccess=1;
#else
	cfg.publicaccess=0;
#endif

#ifdef TIGHT_ARSED_SECURITY
	cfg.tightarsedsecurity=1;
#else
	cfg.tightarsedsecurity=0;
#endif

#ifdef STEALTH_MODE
	cfg.stealthmode=1;
#else
	cfg.stealthmode=0;
#endif

	strcpy(cfg.nick,NICK);
	strcpy(cfg.userinfo,USERINFO);
	strcpy(cfg.stealthcomment,STEALTH_COMMENT);
	strcpy(cfg.stealthversion,STEALTH_VERSION);

	if (!rehashed)
	{
		strcpy(cfg.bothome,expand_tilde(BOTHOME));
		strcpy(cfg.filedir,expand_tilde(FILEDIR));
		strcpy(cfg.userfile,expand_tilde(USERFILE));
		strcpy(cfg.logfile,expand_tilde(LOGFILE));
		strcpy(cfg.helpfile,expand_tilde(HELPFILE));
		strcpy(cfg.motdfile,expand_tilde(MOTDFILE));
		purify_path(cfg.bothome);
		purify_path(cfg.filedir);
		purify_path(cfg.userfile);
		purify_path(cfg.logfile);
		purify_path(cfg.helpfile);
		purify_path(cfg.motdfile);
	}
	if (!rehashed)
	{
		cfg.servlist=NULL;
	}
	cfg.dcctimeout=DCC_TIMEOUT;
	cfg.chattimeout=CHAT_TIMEOUT;
	cfg.pingtimeout=PING_TIMEOUT;
	cfg.dcclimit=DCCLIMIT;
	cfg.dccblocksize=DCC_BLOCK_SIZE;
	cfg.scheduledwrite=SCHEDULED_WRITE;
	cfg.defaultflags=DEFAULT_FLAGS;
	cfg.defaultratio=DEFAULT_RATIO;

	if (!read_configfile(u))
	{
		if (u)
		{
			uprintf(u,"Unable to open config file! Using default values.\n");
		}
		else
		{
			fprintf(stderr,"Unable to open config file! Using default values.\n");
		}
	}

	if (!cfg.servlist)
	{
		if (!(cfg.servlist=(struct server_t *)malloc(sizeof(struct server_t))))
		{
			/* This hardly ever happens. */
			fprintf(stderr,"Aigh! Unable to allocate memory! (This is bad.)\n");
			exit(1);
		}
		memset(cfg.servlist,0,sizeof(struct server_t));
		strcpy(cfg.servlist->name,SERVER);
		cfg.servlist->port=PORT;
		cfg.servlist->next=NULL;
	}
	rehashed=1;
}


/* Function: Read and interprete the config file. */
int read_configfile(struct chat_t *u)
{
	int linenum=1;
	FILE *cfgfile;
	static int rehashed=0;
	long fsize;
	char *memptr,*end,*p,*q;

	if (!(cfgfile=fopen(cfg.configfile,"r"))) return(0);
	
	fseek(cfgfile,0,SEEK_END);
	fsize=ftell(cfgfile); /* Check the file size */
	rewind(cfgfile);

	if (!(memptr=(char *)malloc(fsize+1)))
	{
		fclose(cfgfile);
		return(0);
	}
	end=memptr+fsize;
	fread(memptr,1,fsize,cfgfile);
	fclose(cfgfile);
	p=q=memptr;
	
	/* Remove those annoying tabs. */
	while (q<end)
	{
		if (*q=='\t') *q=' ';
		q++;	
	}
	
	while (p<end)
	{
		/* Find first non-whitespace character. */
		while (p<end&&*p==' ') p++;

		if (*p=='\n')
		{
			p++;
			linenum++;
		}
		else if (*p=='#')
		{
			while(p<end&&*p!='\n') p++; /* Seek to end of line. */
		}
		else
		{
			q=p;
			while(p<end&&*p!='\n') p++; /* Seek to end of line */
			while(p>q&&(*p==' '||*p=='\n')) p--; /* Seek backwards */
			if (p>q)
			{
				*++p='\0'; /* Mark end of line. */
				p++; /* Point to first character after end of line. */
				if (!parse_config_line(q,rehashed))
				{
					if (u)
					{
						uprintf(u,"Config error at line %d: %s\n",linenum,q);
					}
					else
					{
						fprintf(stderr,"Config error at line %d: %s\n",linenum,q);
					}
				}
			}
			linenum++;
		}
	}
	free(memptr);
	rehashed=1;
	return(1);
}

/* Function: Parses a single line from the config file. */
int parse_config_line(char *line,int r)
{
	int idx;

	char **av;
	
	if (!(av=parse_line(line))||!av[1]) return(0);

	for (idx=0;cfgfunc[idx].tag&&strcasecmp(av[0],cfgfunc[idx].tag);idx++);
	if (!cfgfunc[idx].tag)
	{
		free_argv(av);
		return(0);
	}
	if (cfgfunc[idx].func)
	{
		r=cfgfunc[idx].func(av+1,r);
		free_argv(av);
		return(r);
	}
	free_argv(av);
	return(1);
}

/* Function: Expands any occurrences of tilde (~) to full path. */
char *expand_tilde(char *in)
{
	static char out[NAME_MAX+1],buf[NAME_MAX+1],*p;
	struct passwd *pw;

	strcpy(buf,in+1);
	if (in[0]!='~')
	{
		strcpy(out,in);
	}
	else if (in[1]=='/')
	{
		if (!(pw=getpwuid(getuid())))
		{
			strcpy(out,in);
		}
		else
		{
			strcpy(out,pw->pw_dir);
			strcat(out,&in[1]);
		}
	}
	else if (!(p=strtok(buf,"/")))
	{
		strcpy(out,in);
	}
	else if (!(pw=getpwnam(p)))
	{
		strcpy(out,in);
	}
	else
	{
		strcpy(out,pw->pw_dir);
		strcat(out,strstr(in,"/"));
	}
	return(out);
}

int cfg_nick(char **av,int r)
{
	if (!isvalidnick(av[0])) return(0);
	mystrncpy(cfg.nick,av[0],NICK_LEN);
	return(1);
}

int cfg_userinfo(char **av,int r)
{
	mystrncpy(cfg.userinfo,av[0],USERINFO_LEN);
	return(1);
}

int cfg_stealthcomment(char **av,int r)
{
	mystrncpy(cfg.stealthcomment,av[0],MSG_LEN);
	return(1);
}

int cfg_stealthversion(char **av,int r)
{
	mystrncpy(cfg.stealthversion,av[0],MSG_LEN);
	return(1);
}

int cfg_server(char **av,int r)
{
	int tmp=0;

	if (!av[0]) return(0);
	if (av[1])
	{
		tmp=atoi(av[1]);
		if (tmp<1||tmp>65535) return(0);
	}
	else tmp=PORT; /* If no port is given, use the default. */
	add_server(av[0],tmp);
	return(1);
}

int cfg_bothome(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.bothome,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_filedir(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.filedir,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_userfile(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.userfile,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_logfile(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.logfile,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_helpfile(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.helpfile,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_motdfile(char **av,int r)
{
	if (!av[0]) return(0);
	purify_path(av[0]);
	if (!r) mystrncpy(cfg.motdfile,expand_tilde(av[0]),NAME_MAX);
	return(1);
}

int cfg_reconnect(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.reconnect=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.reconnect=0;
		return(1);
	}
	else return(0);
}

int cfg_serverwalking(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.walking=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.walking=0;
		return(1);
	}
	else return(0);
}

int cfg_overwrite(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.overwrite=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.overwrite=0;
		return(1);
	}
	else return(0);
}

int cfg_publicaccess(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.publicaccess=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.publicaccess=0;
		return(1);
	}
	else return(0);
}

int cfg_tightarsedsecurity(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.tightarsedsecurity=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.tightarsedsecurity=0;
		return(1);
	}
	else return(0);
}

int cfg_stealthmode(char **av,int r)
{
	if (!strcasecmp(av[0],"Yes"))
	{
		cfg.stealthmode=1;
		return(1);
	}
	else if (!strcasecmp(av[0],"No"))
	{
		cfg.stealthmode=0;
		return(1);
	}
	else return(0);
}

int cfg_loglevel(char **av,int r)
{
	int i;
	unsigned long tmp2=0;

	for (i=0;av[i];i++)
	{
		if (!strcasecmp(av[i],"ALL"))        tmp2  = ALL;
		else if (!strcasecmp(av[i],"NONE"))  tmp2  = NONE;
		else if (!strcasecmp(av[i],"CMD"))   tmp2 |= CMD;
		else if (!strcasecmp(av[i],"SERV"))  tmp2 |= SERV;
		else if (!strcasecmp(av[i],"DCC"))   tmp2 |= DCC;
		else if (!strcasecmp(av[i],"CTCP"))  tmp2 |= CTCP;
		else if (!strcasecmp(av[i],"SIG"))	 tmp2 |= SIG;
		else if (!strcasecmp(av[i],"MSG"))	 tmp2 |= MSG;
		else if (!strcasecmp(av[i],"CRAP"))	 tmp2 |= CRAP;
		else if (!strcasecmp(av[i],"-CMD"))  tmp2 &= ~CMD;
		else if (!strcasecmp(av[i],"-SERV")) tmp2 &= ~SERV;
		else if (!strcasecmp(av[i],"-DCC"))  tmp2 &= ~DCC;
		else if (!strcasecmp(av[i],"-CTCP")) tmp2 &= ~CTCP;
		else if (!strcasecmp(av[i],"-SIG"))  tmp2 &= ~SIG;
		else if (!strcasecmp(av[i],"-MSG"))  tmp2 &= ~MSG;
		else if (!strcasecmp(av[i],"-CRAP")) tmp2 &= ~CRAP;
		else return(0);
	}
	cfg.loglevel=tmp2;
	return(1);
}

int cfg_dcctimeout(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.dcctimeout=tmp*60;
	return(1);
}

int cfg_chattimeout(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.chattimeout=tmp*60;
	return(1);
}

int cfg_pingtimeout(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.pingtimeout=tmp;
	return(1);
}

int cfg_dcclimit(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.dcclimit=dcclimit=tmp;
	return(1);
}

int cfg_dccblocksize(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.dccblocksize=tmp<BIG_BUFFER_SIZE?BIG_BUFFER_SIZE:tmp;
	return(1);
}

int cfg_scheduledwrite(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.scheduledwrite=tmp;
	return(1);
}

int cfg_defaultflags(char **av,int r)
{
	int tmp=0,i;

	for (i=0;av[i];i++)
	{
		if (!strcasecmp(av[i],"ALL"))			tmp  = F_ALL;
		else if (!strcasecmp(av[i],"NONE"))		tmp  = F_NONE;
		else if (!strcasecmp(av[i],"AUTO"))		tmp |= F_AUTO;
		else if (!strcasecmp(av[i],"SORT"))		tmp |= F_SORT;
		else if (!strcasecmp(av[i],"MOTD"))		tmp |= F_MOTD;
		else if (!strcasecmp(av[i],"RSUM"))		tmp |= F_RSUM;
#if ENABLE_TAR
		else if (!strcasecmp(av[i],"TAR"))		tmp |= F_TAR;
#endif
		else if (!strcasecmp(av[i],"-AUTO"))	tmp &= ~F_AUTO;
		else if (!strcasecmp(av[i],"-SORT"))	tmp &= ~F_SORT;
		else if (!strcasecmp(av[i],"-MOTD"))	tmp &= ~F_MOTD;
		else if (!strcasecmp(av[i],"-RSUM"))	tmp &= ~F_RSUM;
#if ENABLE_TAR
		else if (!strcasecmp(av[i],"-TAR"))		tmp &= ~F_TAR;
#endif
		else return(0);
	}
	cfg.defaultflags=tmp;
	return(1);
}

#ifdef ENABLE_RATIOS
int cfg_defaultratio(char **av,int r)
{
	int tmp=0;

	if (!av[0]||(tmp=atoi(av[0]))<0)
	{
		return(0);
	}
	cfg.defaultratio=tmp;
	return(1);
}
#endif /* ENABLE_RATIOS */

