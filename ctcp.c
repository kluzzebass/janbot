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
 * $Id: ctcp.c,v 1.3 1998/12/09 19:06:05 kluzz Exp $
 */


#include <janbot.h>

/* List of CTCP requests to handle. Not alpabetically stored. Yet. */
struct ctcp_t ctcplist[]={
{ "ACTION",	"contains action descriptions for atmosphere",NULL,NULL },
{ "AUTH",	NULL,ctcp_auth,ctcp_auth },
{ "CLIENTINFO",	"gives information about available CTCP commands",NULL,ctcp_clientinfo },
{ "DCC",	"requests a direct_client_connection",ctcp_dcc,ctcp_dcc },
{ "ECHO",	"returns the arguments it receives",NULL,ctcp_echo },
{ "ERRMSG",	"returns error messages",NULL,ctcp_echo },
{ "FINGER",	"shows real name, login name and idle time of user",NULL,ctcp_finger },
{ "HELP",	"displays list of bot specific CTCP commands",ctcp_help,NULL },
{ "IDENT",	NULL,ctcp_ident,ctcp_ident },
{ "KILLME",	NULL,ctcp_killme,ctcp_killme },
{ "PING",	"returns the arguments it receives",ctcp_echo,ctcp_echo },
{ "SED",	"contains simple_encrypted_data",NULL,NULL },
{ "TIME",	"tells you the time on the user's host",NULL,ctcp_time },
{ "USERINFO",	"returns user settable information",NULL,ctcp_userinfo },
{ "UTC",	"substitutes the local timezone",NULL,NULL },
{ "VERSION",	"shows client type, version and environment",ctcp_version_normal,ctcp_version_stealth },
{ NULL,		NULL,NULL,NULL }
};



/* Function: Parses a CTCP from a client. */
void parse_ctcp(char *orig, char *args)
{
	char *cmd=args;
	int i;
	void (*func)();
	

	for (;(*args!=' ')&&(*args!='\0');args++);
	if (*args==' ') *args++='\0';

	log(CTCP,"CTCP %s from %s.",cmd,orig);

	for (i=0;ctcplist[i].name&&strcasecmp(ctcplist[i].name,cmd);i++);
	if (!ctcplist[i].name)
	{
		send_to_server("NOTICE %s :\001ERRMSG Unknown CTCP query \"%s\"\001\n",getnick(orig),cmd);
		return;
	}
	
	/* Call the appropriate CTCP handler. If the handler is NULL, drop it.*/
	func=cfg.stealthmode?ctcplist[i].stealth:ctcplist[i].normal;
	if (func) (func)(ctcplist[i].name,orig,args);
}

/* Function: Parses a CTCP_REPLY from a client. */
void parse_ctcp_reply(char *orig, char *args)
{
	/* We do not act on CTCP_REPLY, it could cause loops. */
	/* JanBot does not produce any CTCP requests apart from */
	/* DCC SEND and DCC CHAT, thus it's safe to assume that */
	/* any CTCP replies are fake (as SEND and CHAT should not */
	/* be replied to other than with a connection). */
	DEBUG2("CTCP_REPLY from %s: %s\n",getnick(orig),args);
	log(CTCP,"CTCP_REPLY from %s: ",orig,args);
}

/* Function: Sends a CTCP of a given type to a client. */
void send_ctcp(int type,char *req,char *dest,char *fmt,...)
{
	va_list ap;
	char msg[MSG_LEN+1];
	char *typestr;

	switch (type)
	{
	case CTCP_REQUEST:
		typestr="PRIVMSG";
		break;
	case CTCP_REPLY:
	default:
		typestr="NOTICE";
		break;
	}

	va_start(ap,fmt);
	vsprintf(msg,fmt,ap);
	va_end(ap);
	send_to_server("%s %s :\001%s %s\001\n",typestr,getnick(dest),req,msg);
}

/* Function: Handle for CTCP ECHO, PING and ERRMSG. */
void ctcp_echo(char *req,char *orig,char *args)
{
	send_ctcp(CTCP_REPLY,req,orig,args);
}

/* Function: Handle for CTCP CLIENTINFO. */
void ctcp_clientinfo(char *req,char *orig,char *args)
{
	int i;
	char buf[MSG_LEN+1];
	*buf='\0';
	
	if (strlen(args))
	{
		for (i=0;ctcplist[i].name;i++)
		{
			if (ctcplist[i].stealth) sprintf(buf,"%s %s",buf,ctcplist[i].name);
		}
		strcat(buf,"  :Use CLIENTINFO <COMMAND> to get more specific information");
		send_ctcp(CTCP_REPLY,req,orig,buf);
	}
	else
	{
		for (i=0;ctcplist[i].name&&strcmp(ctcplist[i].name,args);i++);
		if (ctcplist[i].name&&ctcplist[i].help) send_ctcp(CTCP_REPLY,req,orig,"%s %s",ctcplist[i].name,ctcplist[i].help);
		else send_ctcp(CTCP_REPLY,"ERRMSG",orig,"%s is not a valid function",args);
	}
}

/* Function: Handle for CTCP DCC. */
void ctcp_dcc(char *req,char *orig,char *args)
{
	incoming_dcc(orig,args);
}

/* Function: Handle for CTCP FINGER. */
void ctcp_finger(char *req,char *orig,char *args)
{
	struct passwd *pwd;
	char *tmp;
	time_t diff;
	
	diff=time(NULL)-idle_time;
	if ((pwd=getpwuid(getuid())))
	{
		if ((tmp=strstr(pwd->pw_gecos,","))) *tmp='\0';
		send_ctcp(CTCP_REPLY,req,orig,"%s (%s@%s) Idle %ld second%c",pwd->pw_gecos,pwd->pw_name,hostname,diff,diff==1?"":"s" );
	}
}

/* Function: Handle for CTCP TIME. */
void ctcp_time(char *req,char *orig,char *args)
{
	time_t tm=time(NULL);
	char *s,*t=ctime(&tm);

	if ((s=index(t,'\n'))) *s = '\0';
	send_ctcp(CTCP_REPLY,req,orig,t);
}

/* Function: Handle for CTCP USERINFO. */
void ctcp_userinfo(char *req,char *orig,char *args)
{
	send_ctcp(CTCP_REPLY,req,orig,cfg.userinfo);
}

/* Function: Handles the CTCP VERSION request in normal mode. */
void ctcp_version_normal(char *req,char *orig,char *args)
{
	send_ctcp(CTCP_REPLY,req,orig,BOTVERSION);
}

/* Function: Handles the CTCP VERSION request in stealth mode. */
void ctcp_version_stealth(char *req,char *orig,char *args)
{
	struct utsname u;
	char *usys,*uver;

	if (uname(&u))
	{
		usys="unknown";
		uver="";
	}
	else
	{
		usys=u.sysname;
		uver=u.version;
	}
	send_ctcp(CTCP_REPLY,req,orig,"ircII %s %s %s :%s\n",cfg.stealthversion,usys,uver,cfg.stealthcomment);
}

/* Function: Handle for CTCP HELP. */
void ctcp_help(char *req,char *orig,char *args)
{
	send_to_server("NOTICE %s :\001HELP \002%s\002 CTCP HELP:\001\n",getnick(orig),cfg.nick);
	send_to_server("NOTICE %s :\001HELP  \002AUTH\002 <passwd>   - Request connection.\001\n",getnick(orig));
	if (!cfg.tightarsedsecurity) send_to_server("NOTICE %s :\001HELP  \002IDENT\002 <passwd>  - Register hostmask.\001\n",getnick(orig));
	send_to_server("NOTICE %s :\001HELP  \002KILLME\002 <passwd> - Terminate connection.\001\n",getnick(orig));
	send_to_server("NOTICE %s :\001HELP End of help.\001\n",getnick(orig));
}

void ctcp_auth(char *req,char *orig,char *args)
{
	char *tmp;
	int ctcperr;

	for (tmp=args;(*tmp!=' ')&&(*tmp!='\0');tmp++);
	if (*tmp==' ') *tmp='\0';
	if (!authenticate(getnick(orig),getuserhost(orig),args,&users))
	{
		if (!(ctcperr=add_chat(getnick(orig),&chatlist)))
		{
			send_to_server("NOTICE %s :\001AUTH Access granted!\001\n",getnick(orig));
		}
		else
		switch (ctcperr)
		{
		/* We have quite extensive error handling... */
		case 3:
			send_to_server("NOTICE %s :\001AUTH Sorry, only one chat allowed.\001\n",getnick(orig));
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			send_to_server("NOTICE %s :\001AUTH Sorry, could not set up chat connection.\001\n",getnick(orig));
			break;
		case 8:
		case 9:
			send_to_server("NOTICE %s :\001AUTH Sorry, unable to set up child communication channel.\001\n",getnick(orig));
			break;
		case 10:
			send_to_server("NOTICE %s :\001AUTH Sorry, could not spawn child process.\001\n",getnick(orig));
			break;
		default:
			send_to_server("NOTICE %s :\001AUTH Sorry, cannot fullfill your wishes.\001\n",getnick(orig));
			break;
		}
	}
	else if (!cfg.stealthmode)
	{
		send_to_server("NOTICE %s :\001AUTH Access denied.\001\n",getnick(orig));
	}			
}

void ctcp_ident(char *req,char *orig,char *args)
{
	char *cmd=args,*tmp,*u,*h,mask[USERHOST_LEN+1],pw[9];
	struct user_t *tmpu;

	if (!cfg.tightarsedsecurity)
	{
		for (tmp=args;(*tmp!=' ')&&(*tmp!='\0');tmp++);
		if (*tmp==' ') *tmp='\0';
		if (!chkpasswd(getnick(orig),args,&users))
		{
			u=strtok(getuserhost(orig),"@");
			if (*u=='^') u++;
			else if (*u=='~') u++;
			else if (*u=='+') u++;
			else if (*u=='=') u++;
			else if (*u=='-') u++;
			h=create_hostmask(strtok(NULL,"@"));
			sprintf(mask,"%s@%s",u,h);
			add_userhost(getnick(orig),mask,&users);
			send_to_server("NOTICE %s :\001IDENT Okay, I now recognize you from \002%s\002.\001\n",getnick(orig),mask);
		}
		else
		{
			if (cfg.publicaccess)
			{
				if ((tmpu=get_user(getnick(orig),&users)))
				{
					send_to_server("NOTICE %s :\001IDENT Exsqueeze me? Baking powder? Do I know you?\001\n",getnick(orig));
					return;
				}
				if (!(tmpu=add_user(getnick(orig),&users)))
				{
					send_to_server("NOTICE %s :\001IDENT Sorry, unable to register you as a new user.\001\n",getnick(orig));
					return;
				}
				if (!(h=strtok(args," ")))
				{
					send_to_server("NOTICE %s :\001IDENT Please supply a password.\001\n",getnick(orig));
					return;
				}
				mystrncpy(pw,h,8);
				setpasswd(getnick(orig),strtok(args," "),&users);
				u=strtok(getuserhost(orig),"@");
				if (*u=='^') u++;
				else if (*u=='~') u++;
				else if (*u=='+') u++;
				else if (*u=='=') u++;
				else if (*u=='-') u++;
				h=create_hostmask(strtok(NULL,"@"));
				sprintf(mask,"%s@%s",u,h);
				add_userhost(getnick(orig),mask,&users);
				send_to_server("NOTICE %s :Hi there! You have just been added to my userlist\n",getnick(orig));
				send_to_server("NOTICE %s :with the hostmask \002%s\002. To connect you must\n",getnick(orig),mask);
				send_to_server("NOTICE %s :always use your current nick, issue the command\n",getnick(orig));
				send_to_server("NOTICE %s :'/CTCP %s AUTH \002%s\002', and use the chat to play.\n",getnick(orig),cfg.nick,pw);
				send_to_server("NOTICE %s :\002%s\002 is your password, and it can be changed later.\n",getnick(orig),pw);
			}
			else
			{
				send_to_server("NOTICE %s :\001IDENT Exsqueeze me? Baking powder? Do I know you?\001\n",getnick(orig));
			}
		}
	}
	else
	{
		send_to_server("NOTICE %s :\001ERRMSG Unknown CTCP query \"%s\"\001\n",getnick(orig),cmd);
	}
}

void ctcp_killme(char *req,char *orig,char *args)
{
	char *tmp;
	struct chat_t *tmpc;

	for (tmp=args;(*tmp!=' ')&&(*tmp!='\0');tmp++);
	if (*tmp==' ') *tmp='\0';
	if (!authenticate(getnick(orig),getuserhost(orig),args,&users))
	{
		if ((tmpc=find_chat(get_user(getnick(orig),&users),&chatlist)))
		{
			kill_chat(tmpc,&chatlist);
			send_to_server("NOTICE %s :\001KILLME Connection has been terminated.\001\n",getnick(orig));
		}
		else
		{
			send_to_server("NOTICE %s :\001KILLME No connection found.\001\n",getnick(orig));
		}
	}
}

