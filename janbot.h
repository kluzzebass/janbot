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
 * $Id: janbot.h,v 1.7 1998/11/22 01:05:12 kluzz Exp $
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <sys/socket.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <glob.h>
#include <ctype.h>
#include <stdlib.h>
#include <pwd.h>

/* Prefer sys/statfs.h over sys/vfs.h. What happens if neither exists is undetermined. */
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#else
# ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
# endif
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#include <stdarg.h>

#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_FNMATCH
# include <fnmatch.h>
#else
# error "I really need fnmatch() to work."
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif


/* Solaris systems pre 2.6 seems to have no gethostname() prototype. */
#ifdef HAVE_NO_GETHOSTNAME_PROTO
extern int gethostname(char *, int);
#endif


/* And that concludes the include section. */



/* User serviceable defines */

/* Some initial values. Most of these can be overridden in the config file. */

/* Some flags that might be useful. */
#define RECONNECT
#define SERVER_WALKING
#define OVERWRITE
#undef PUBLIC_ACCESS
#undef TIGHT_ARSED_SECURITY
#undef STEALTH_MODE

/* The standard nick for this bot. */
#define NICK "JanBot"
/* The userinfo for the bot. */
#define USERINFO "/CTCP JanBot HELP"
/* The default server and port. If any servers are found */
/* in the config file, this server will be skipped. */
#define SERVER "irc.pasta.no"
#define PORT 6667

/* These are only used when JanBot is running in Stealth Mode. */
#define STEALTH_COMMENT "this is a bug free client.  honest"
#define STEALTH_VERSION "2.9_base"

/* Bot homedir and filedir. These must be absolute paths. */
#define BOTHOME "/home/janbot"
#define FILEDIR "/home/janbot/files"

/* Some filenames */
#define USERFILE "janbot.usr"
#define LOGFILE "janbot.log"
#define HELPFILE "janbot.hlp"
#define MOTDFILE "janbot.msg"



/* CHANGES BELOW THIS LINE WILL VOID WARRANTY! */

/* The name of the defalt config file. It must */
/* be located in the users $HOME directory. */
#define CONFIGFILE ".janbotrc"

/* Some sizes, should not be changed unless you know what you're doing. */
#define NICK_LEN 9
#define USERHOST_LEN 50
#define USERDIR_LEN 50
#define SERVER_LEN 40
#define USERINFO_LEN 30
/* How many seconds should the bot offer a DCC CHAT/SEND before closing it? */
#define DCC_TIMEOUT 60*2 /* Currently 2 minutes */
/* How many seconds can a user be idle with 0 DCCs before closing the chat? */
#define CHAT_TIMEOUT 60*15 /* Currently 15 minutes */
/* How many seconds should we allow the server to be idle until we switch? */
#define PING_TIMEOUT 60*3 /* Currently 3 minutes */
/* Default DCC limit for each user. Can be changed at run time. */
#define DCCLIMIT 5
/* Default time (in seconds) between every time the userfile is written. */
#define SCHEDULED_WRITE 600
/* Default flags for new users. */
#define DEFAULT_FLAGS F_AUTO|F_SORT|F_MOTD|F_RSUM
/* Default Upload/Download ratio. */
#define DEFAULT_RATIO 5
/* Define if you want the bot to be publically accessible by default. */
#undef PUBLIC_ACCESS



/* The current version of the bot. Don't change this. */
#define BOTVERSION "JanBot-3.08beta4"

/* The standard buffer size in the irc client. It's a good value to use. */
#define BIG_BUFFER_SIZE 2048
/* This is defines the default buffer size for the hyperdcc technology. */
#define DCC_BLOCK_SIZE 4096

/* Standard message size */
#define MSG_LEN 512

/* Length of an IPC message. */
#define IPC_MSG_LEN 256
#define IPC_TXT_LEN IPC_MSG_LEN-sizeof(int)-1

/* All these defines are confusing... :D */
#ifndef NAME_MAX
# ifdef MAXNAMLEN
#  define NAME_MAX MAXNAMLEN
# else
#  define NAME_MAX 256
# endif
#endif

#ifndef OPEN_MAX
# ifdef _NFILE
#  define OPEN_MAX _NFILE
# else
#  define OPEN_MAX getdtablesize()
# endif
#endif


/* On some hosts this is not defined. Or maybe it's hidden in an include file I'm missing? */
#ifndef INADDR_NONE
# define INADDR_NONE ((unsigned long) 0xffffffff)
#endif

/* JanBot can't really run on NT. Or can it? Let's see what Cygnus Solutions comes up with. */
#ifdef _WINDOWS_NT_
# define POOR_MANS_SOCKETPAIR
#endif /* _WINDOWS_NT_ */


/* These are process name postfixes. */
#define MAIN_PROCESS " (main process)"
#define DCC_CHAT_CHILD " (user process)"
#define USERLIST_CHILD " (storing userlist)"

/* Userlist related stuff */

/* The 4 levels of enlightenment... Yeah, I love the Enlightenment WM :) */
#define NORMAL_USER 0
#define POWER_USER 1
#define MASTER_USER 2
#define SUPERIOR_BEING 3

struct user_t
{
	char nick[NICK_LEN+1];
	char passwd[14]; /* This passwd string is stored encrypted. */
	struct list_t *alias;
	struct list_t *userhost;
	int level,dcclimit;
	char dir[USERDIR_LEN+1];
	
	/* These are related to the ratio system. */
	int flags; /* Contains the users personal settings. */
	int ratio; /* Upload/Download ratio. */
	unsigned long long ul; /* Bytes uploaded. */
	unsigned long long dl; /* Bytes downloaded. */
	unsigned long long cr; /* Credit amount. */
	
	/* Next user in list. */
	struct user_t *next;
};


/* The user config flags. There will be more flags later. */
#define F_AUTO 0x0001   /* Automatical DCC SEND/GET if set. */
#define F_SORT 0x0002   /* Sort alphabetically if cleared, by date if set */
#define F_MOTD 0x0004   /* Toggle display of Message Of The Day */
#define F_RSUM 0x0008   /* Send DCC RESUME request if file exists. */
#define F_TAR  0x0010   /* Toggle TAR sending on/off. */
#define F_NONE 0x0000 /* No flags */

#define F_ALL  F_AUTO|F_SORT|F_MOTD|F_RSUM|F_TAR /* All flags. */

/* Server related stuff */

struct server_t
{
	char name[SERVER_LEN+1];
	unsigned int port;
	struct server_t *next;
};

struct servconn_t
{
	time_t lastrecv;
	time_t lastping;
	int servfd;
	struct sockaddr_in serv_addr;
};

/* DCC CHAT related stuff */

struct chat_t
{
	struct user_t *chatter;
	char nick[NICK_LEN+1]; /* Currently used nick. */
	int active,terminate;
	int oldsockfd,sockfd,clilen;
	unsigned int port;
	int ipc[2];
	int childpid;
	int filenum,queued;
	time_t stime,itime;
	struct sockaddr_in srv_addr,cli_addr;
	struct chat_t *next;
};

struct childmsg_t
{
	int type;
	char msg[IPC_TXT_LEN];
};

/* These are the message names for the parent-child communication. */
/* They are used by both entities, but their meaning differs. */

#define ERROR	0	/* Should be pretty obvious */
#define LIST	1	/* Makes the child list all open/waiting dccs */
#define SEND	2	/* Parent: Send this file, Child: this file sent. */
#define TAR	4	/* Send this tarfile. */
#define GET		3	/* Parent: Get this file, Child: this file gotten. (?) */
#define KILL	5	/* Kill a dcc by number or ALL */
#define QLIST	6	/* List files in queue. */
#define QKILL	7	/* Dequeues a dcc by number or ALL */
#define AUTO	8	/* Sets autodcc & resume on/off. */
#define NEXT	9	/* Send next file in DCC queue. */
#define NUM		10	/* From Child to Parent, reporting number of DCCs,
		 from Parent to Child, setting DCC limit. */
#define RESUME	11	/* Incoming resume request. */
#define RESREQ	12	/* Outgoing resume request. */

#ifdef ENABLE_RATIOS
# define RATIO  13	/* For the ratio system. */
#endif

/* DCC types */
#define D_SEND		0x01	/* Send file. */
#define D_GET		0x02	/* Get file. */
#define D_TAR		0x04	/* For sending tar files generated on-the-fly. */
#define D_RESUME	0x08	/* Not needed for anything but DCC GET. */

struct dcc_t
{
	char fname[NAME_MAX+1]; /* Not always the real filename. */
	int sockfd,tmpfd; /* Socket descriptors. */
	FILE *file; /* File to read or write. */
	int type; /* Get, Send, TarSend or Resume, I recon... */
	int active; /* Waiting or Active */
	time_t stime; /* Start time */
	int port,clilen; /* These are SEND related. */
	struct sockaddr_in srv_addr,cli_addr; /* For SEND we need both */
	unsigned long total,transmit; /* Total file size, bytes transmitted. */
	unsigned long resume; /* Total size of resumed file. */
	int bsize; /* DCC Block Size for this particular connection. */
	struct dcc_t *next;
};

/* This is a list of DCCs to be exterminated. */
struct deathrow_t
{
	struct dcc_t *convict;
	struct deathrow_t *next;
};

/* This is for the DCC queue system. */
struct dccqueue_t
{
	int type;
	char info[NAME_MAX+1]; /* Hope this is enough. */
	struct dccqueue_t *next;
};

/* This list is for sorting directory entries. */
struct dirlist_t
{
	char name[NAME_MAX+1];
	struct stat sbuf;
	struct dirlist_t *next;
};


/* Command related stuff */
struct cmd_t
{
	char *name;
	int (*func)();
	int level; /* Required access level. */
	char *usage;
};

/* The CTCP handler structure. */
struct ctcp_t
{
	char *name; /* Name of the CTCP request. */
	char *help; /* Help text, adapted from ircII. */
	void (*normal)();  /* Call this in normal mode. */
	void (*stealth)(); /* Call this in stealth mode. */
};

#define CTCP_REQUEST 1
#define CTCP_REPLY 2

/* This is some oddstuff for the user add command. */
struct uhost_query
{
	char nick[NICK_LEN+1];
	struct user_t *caller;
};

/* Configuration container */
struct cfg_t
{
	char configfile[NAME_MAX+1];
	
	int reconnect;
	int walking;
	int overwrite;
	int publicaccess;
	int tightarsedsecurity;
	int stealthmode;
	
	unsigned long loglevel;
	char nick[NICK_LEN+1];
	char userinfo[USERINFO_LEN+1];
	
	char stealthcomment[MSG_LEN+1];
	char stealthversion[MSG_LEN+1];
	
	char bothome[NAME_MAX+1];
	char filedir[NAME_MAX+1];
	
	char userfile[NAME_MAX+1];
	char logfile[NAME_MAX+1];
	char helpfile[NAME_MAX+1];
	char motdfile[NAME_MAX+1];
	
	struct server_t *servlist;
	
	int dcctimeout;
	int chattimeout;
	int pingtimeout;
	int dcclimit;
	int scheduledwrite;
	int defaultflags;
	int dccblocksize;
	int defaultratio;
};

struct cfgfunc_t
{
	char *tag;
	int (*func)();
};


/* Log related stuff */
#define NONE	0x00
#define CMD		0x01
#define SERV	0x02
#define DCC		0x04
#define CTCP	0x08
#define SIG		0x10
#define MSG		0x20
#define CRAP	0x40
#define ALL		CMD|SERV|DCC|CTCP|SIG|MSG|CRAP


/* Generic string container list */
struct list_t
{
	char *str;
	struct list_t *next;
};

/* Generic file container list */
struct file_t
{
	time_t date;
	unsigned long size;
	char name[NAME_MAX+1];
	char path[NAME_MAX+1];
	struct file_t *next;
};

struct help_t
{
	char cmd[16]; /* Command name. */
	struct helptext_t *text; /* Chain of help lines. */
	struct help_t *next;
};

struct helptext_t
{
	int levelmask;
	char text[80+1];
	struct helptext_t *next;
};

/* Function prototypes */

#ifdef _SOLARIS_
extern int				gethostname(char *name, size_t len);
#endif

/* chat.c */
extern int				add_chat(char *nick,struct chat_t **clist);
extern struct chat_t    *find_chat(struct user_t *u,struct chat_t **clist);
extern void				kill_chat(struct chat_t *the_chat,struct chat_t **clist);
extern void				check_chat(struct chat_t **clist);
extern void				check_idle_time(struct chat_t **clist);
extern void				remove_marked_chats(struct chat_t **clist);
extern void				uprintf(struct chat_t *,char *,...);
extern void				send_to_process(struct chat_t *,int,char *,...);
extern void				parse_child_message(struct chat_t *usr,int message,char *msgtext);
extern void				display_stuff(struct chat_t *usr,char *msgtext);
#ifdef ENABLE_RATIOS
extern void				parse_child_ratio(struct chat_t *usr,char *msgtext);
#endif

/* child.c */
extern void				init_child(struct chat_t *user);
extern int				process_child();
extern void				kill_child();
extern void				parse_parent_message(int message,char *msgtext);
#ifdef ENABLE_DEBUG
extern void				child_signal_handler(int sig);
#endif /* ENABLE_DEBUG */
#ifdef ENABLE_RATIOS
extern void				parse_parent_ratio(char **args);
#endif

/* commands.c */
extern int				cmd_add(struct chat_t *usr,char **args);
#ifdef ENABLE_RATIOS
extern int				cmd_adjust(struct chat_t *usr,char **args);
#endif
extern int	 			cmd_alias(struct chat_t *usr,char **args);
extern int	 			cmd_cd(struct chat_t *usr,char **args);
extern int	 			cmd_cutcon(struct chat_t *usr,char **args);
extern int	 			cmd_del(struct chat_t *usr,char **args);
#ifdef ENABLE_RATIOS
extern int				cmd_donate(struct chat_t *usr,char **args);
#endif
extern int				cmd_help(struct chat_t *usr,char **args);
extern int				cmd_hint(struct chat_t *usr,char **args);
extern int				cmd_killdcc(struct chat_t *usr,char **args);
extern int				cmd_killq(struct chat_t *usr,char **args);
extern int				cmd_limit(struct chat_t *usr,char **args);
extern int	 			cmd_ls(struct chat_t *usr,char **args);
extern int				cmd_mkdir(struct chat_t *usr,char **args);
extern int				cmd_next(struct chat_t *usr,char **args);
extern int				cmd_passwd(struct chat_t *usr,char **args);
extern int				cmd_quit(struct chat_t *usr,char **args);
#ifdef ENABLE_RATIOS
extern int				cmd_ratio(struct chat_t *usr,char **args);
#endif /* ENABLE_RATIOS */
extern int				cmd_rehash(struct chat_t *usr,char **args);
extern int				cmd_relevel(struct chat_t *usr,char **args);
extern int				cmd_retire(struct chat_t *usr,char **args);
extern int				cmd_send(struct chat_t *usr,char **args);
extern int				cmd_sendto(struct chat_t *usr,char **args);
extern int				cmd_server(struct chat_t *usr,char **args);
extern int				cmd_set(struct chat_t *usr,char **args);
extern int				cmd_setpass(struct chat_t *usr,char **args);
extern int				cmd_showdcc(struct chat_t *usr,char **args);
extern int				cmd_showq(struct chat_t *usr,char **args);
extern int				cmd_status(struct chat_t *usr,char **args);
#ifdef ENABLE_DEBUG
extern int				cmd_test(struct chat_t *usr,char **args); /* Only for test purposes. */
#endif /* ENABLE_DEBUG */
extern int				cmd_unalias(struct chat_t *usr,char **args);
extern int				cmd_users(struct chat_t *usr,char **args);
extern int				cmd_who(struct chat_t *usr,char **args);
extern int				cmd_whoami(struct chat_t *usr,char **args);

/* config.c */
extern void				init_config(char *cfgfile,struct chat_t *u);
extern int				read_configfile(struct chat_t *u);
extern int				parse_config_line(char *line,int r);
extern char				*expand_tilde(char *in);
extern int				cfg_nick(char **av,int r);
extern int				cfg_userinfo(char **av,int r);
extern int				cfg_stealthcomment(char **av,int r);
extern int				cfg_stealthversion(char **av,int r);
extern int				cfg_server(char **av,int r);
extern int				cfg_bothome(char **av,int r);
extern int				cfg_filedir(char **av,int r);
extern int				cfg_userfile(char **av,int r);
extern int				cfg_logfile(char **av,int r);
extern int				cfg_helpfile(char **av,int r);
extern int				cfg_motdfile(char **av,int r);
extern int				cfg_reconnect(char **av,int r);
extern int				cfg_serverwalking(char **av,int r);
extern int				cfg_overwrite(char **av,int r);
extern int				cfg_tightarsedsecurity(char **av,int r);
extern int				cfg_publicaccess(char **av,int r);
extern int				cfg_stealthmode(char **av,int r);
extern int				cfg_loglevel(char **av,int r);
extern int				cfg_dcctimeout(char **av,int r);
extern int				cfg_chattimeout(char **av,int r);
extern int				cfg_pingtimeout(char **av,int r);
extern int				cfg_dcclimit(char **av,int r);
extern int				cfg_dccblocksize(char **av,int r);
extern int				cfg_scheduledwrite(char **av,int r);
extern int				cfg_defaultflags(char **av,int r);
#ifdef ENABLE_RATIOS
extern int				cfg_defaultratio(char **av,int r);
#endif /* ENABLE_RATIOS */

/* ctcp.c */
extern void				parse_ctcp(char *orig, char *args);
extern void				parse_ctcp_reply(char *orig, char *args);
extern void				send_ctcp(int type,char *req,char *dest,char *fmt,...);
extern void				ctcp_auth(char *req,char *orig,char *args);
extern void				ctcp_clientinfo(char *req,char *orig,char *args);
extern void				ctcp_dcc(char *req,char *orig,char *args);
extern void				ctcp_echo(char *req,char *orig,char *args);
extern void				ctcp_finger(char *req,char *orig,char *args);
extern void				ctcp_help(char *req,char *orig,char *args);
extern void				ctcp_ident(char *req,char *orig,char *args);
extern void				ctcp_killme(char *req,char *orig,char *args);
extern void				ctcp_time(char *req,char *orig,char *args);
extern void				ctcp_userinfo(char *req,char *orig,char *args);
extern void				ctcp_version_normal(char *req,char *orig,char *args);
extern void				ctcp_version_stealth(char *req,char *orig,char *args);


/* dcc.c */
extern void				incoming_dcc(char *orig,char *args);
extern void				open_incoming_file(char *orig,char *args);
extern void				process_outgoing_file(char **mv,int dcctype);
extern void				process_incoming_file(char **mv);
extern int				dcc_count();
extern int				queue_count();
extern void				put_on_deathrow(struct dcc_t *convict);
extern void				clear_deathrow();
extern void				check_dcc();
extern void				process_dccget(struct dcc_t *tmp);
extern void				process_dccsend(struct dcc_t *tmp);
extern void				send_dcc_offer(char *dccinfo);
extern void				check_dcc_timeout();
extern void				put_on_dccqueue(int type,char **mv);
extern void				check_dccqueue();
extern void				show_dcc_list(char **mv);
extern int				dcc_exists(char *filename);
extern int				is_in_queue(char *filename);
extern void				show_dcc_queue(char **mv);
extern void				remove_dcc(char **mv);
extern void				remove_queued(char **mv);
extern void				process_next(char **mv);
extern void				parse_resume_request(char *orig,char *args);
extern void				process_resume(char **mv);

/* fs.c */
extern char				*base_name(char *path);
extern char				*safe_name(char *n);
extern char				*mode2str(unsigned int md);
extern struct dirlist_t	*create_dirlist(DIR *thedir);
extern struct file_t	*create_reclist(char *dir);
extern int				check_dir(char *dirname);
extern int				dirlist_compare(); /* To prevent the compiler from bitching, strict typing skipped. */
#if ENABLE_TAR
extern unsigned long	calculate_tarsize(char *dir);
#endif
extern char				*gethomepath();
extern void				purify_path(char *path);
extern char				*resolve_path(char *path);
extern char				*print_file_stats(struct dirlist_t *fp,char *pname);
extern struct dirlist_t	*strfdup(char *f);
extern int				dlcmp(struct dirlist_t *s1,struct dirlist_t *s2,int sorttype);
#ifdef HAVE_STATFS
extern char				*show_free_space(char *path);
#endif

/* janbot.c */
extern int				do_bot_things();
extern void				show_cli_help();
extern void				signal_handler(int sig);
extern void				sighup_handler(int sig);
extern void				daemonize();

/* parseline.c */
extern char				**parse_line(char *str);
extern char				*unescape(char *in,int *l);
extern void				free_argv(char **av);
extern char				*enquote(char *in);

/* parser.c */
extern int				read_server_line(char *buff);
extern int				parse_server_line(char *buff);
extern void				parse_server_message(char *orig,char *cmd,char *args);
extern void				parse_client_message(char *orig,char *cmd,char *args);
extern void				parse_privmsg(char *orig, char *args);
extern void				parse_notice(char *orig, char *args);
extern void				parse_command(struct chat_t *tmp,char *buffer);

/* server.c */
extern int				server_connect(struct server_t *desc);
extern int				server_init();
extern struct server_t	*next_server();
extern void				add_server(char sname[SERVER_LEN],int sport);
extern char				*del_server(int num);
extern void				send_to_server(char *,...);

/* stuff.c */
extern void				ms_delay(int msec);
extern char				*getnick(char *origin);
extern char				*getuserhost(char *origin);
extern char				*toolow();
extern char				*convert_date(time_t t);
extern char				*convert_idle(time_t t);
extern char				*convert_uptime(time_t t);
extern char				*create_hostmask(char *h);
extern void				showusage(struct chat_t *usr,char *cmd);
extern void				showhelp(struct chat_t *usr,char *topic);
extern void				log(unsigned long,char *,...);
extern unsigned int		next_port();
extern int				isvalidnick(char *n);
extern int				readytoread(int fd);
extern void				*mymalloc(size_t size);
extern char				*mystrncpy(char *dest,char *src,int n);
extern void				lfree(struct list_t *l);
extern struct list_t	*ldup(char *s);
#ifdef POOR_MANS_SOCKETPAIR
extern int				socketpair(int d, int type, int protocol, int sv[2]);
#endif

/* userlist.c */
extern struct user_t	*add_user(char *nickname,struct user_t **ulist);
extern struct user_t	*get_user(char *nickname,struct user_t **ulist);
extern struct user_t	*get_alias(char *nickname,struct user_t **ulist);
extern void				free_users(struct user_t **ulist);
extern void				del_user(char *nickname,struct user_t **ulist);
extern int				initusers();
extern int				read_users(struct user_t **ulist);
extern void				write_scheduled(struct user_t **ulist);
extern void				write_users(struct user_t **ulist);
extern int				write_single_user(struct user_t *tmp);
extern void				add_userhost(char *nick,char *hostmask,struct user_t **ulist);
extern void				del_userhost(char *nick,int num,struct user_t **ulist);
extern int				userhost_count(struct list_t *p);
extern void				add_alias(char *nick,char *alias,struct user_t **ulist);
extern void				del_alias(char *nick,int num,struct user_t **ulist);
extern int				alias_count(struct list_t *p);
extern int				authenticate(char *a_nick,char *a_userhost,char *a_passwd,struct user_t **ulist);
extern int				setpasswd(char *nick,char *passwd,struct user_t **ulist);
extern int				chkpasswd(char *nick,char *passwd,struct user_t **ulist);
extern int				setlevel(char *nick,int lvl,struct user_t **ulist);
extern int				setlimit(char *nick,int lim,struct user_t **ulist);
extern void				parse_userhost_reply(char *args);
extern void				enter_new_user();


/* GLOBALS VARIABLES */

extern struct user_t *users;

extern struct server_t *current_server;
extern struct servconn_t sconn;
extern struct servmsg_t *msgqueue;

extern struct chat_t *chatlist;

extern struct cfg_t cfg;

extern time_t bot_uptime;
extern time_t idle_time;

extern int background;

/* These are for the child processes */
extern struct dcc_t *dcclist;
extern struct deathrow_t *deathrow;
extern struct dccqueue_t *dccqueue;
extern FILE *userfile;
extern FILE *logfile;
extern int parent;
extern int dcclimit;
extern int autodcc;
extern int resume;

#ifdef ENABLE_DEBUG
extern int traceflag;
extern int function_level;
#endif /* ENABLE_DEBUG */

extern struct in_addr hostaddr;
extern char hostname[256];
extern unsigned long local_ip;
extern struct hostent *local_hp;

extern struct cmd_t cmdlist[];
extern struct ctcp_t ctcplist[];
extern char *toolowlist[];

/* Might get extended to a table or list in the future. */
extern struct uhost_query uhquery;


/* For publishing the argument list. */
extern int p_argc;
extern char **p_argv;


#ifdef ENABLE_RATIOS
extern int ratio;
extern unsigned long long ul;
extern unsigned long long dl;
extern unsigned long long cred;
#endif


/* Some debugging stuff */

#ifdef ENABLE_DEBUG

#define DEBUG0(txt) if (traceflag) { printf(txt); }
#define DEBUG1(txt,arg1) if (traceflag) { printf(txt,arg1); }
#define DEBUG2(txt,arg1,arg2) if (traceflag) { printf(txt,arg1,arg2); }
#define DEBUG3(txt,arg1,arg2,arg3) if (traceflag) { printf(txt,arg1,arg2,arg3); }

#else

#define DEBUG0(txt)
#define DEBUG1(txt,arg1)
#define DEBUG2(txt,arg1,arg2)
#define DEBUG3(txt,arg1,arg2,arg3)

#endif /* ENABLE_DEBUG */


