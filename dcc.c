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
 * $Id: dcc.c,v 1.5 1998/11/08 23:36:07 kluzz Exp $
 */

	
#include <janbot.h>


struct dcc_t *dcclist=NULL;
struct deathrow_t *deathrow=NULL;
struct dccqueue_t *dccqueue=NULL;

int dcclimit=10;
int autodcc=1;
int resume=0;


/* Function: Parse and process an incoming DCC. */
void incoming_dcc(char *orig,char *args)
{
	char *cmd;
	for (cmd=args;(*args!=' ')&&(*args!='\0');args++);
	*args++='\0';

	if (!strcasecmp("SEND",cmd))
	{
		open_incoming_file(orig,args);
		return;
	}
	else if (!strcasecmp("RESUME",cmd))
	{
		parse_resume_request(orig,args);
		return;
	}
	else if (!strcasecmp("CHAT",cmd))
	{
		send_to_server("NOTICE %s :\001ERRMSG I do not accept DCC CHAT without authorization. Use '/CTCP %s AUTH <passwd>' to connect.\001\n",getnick(orig),cfg.nick);
	}
	else
	{
		send_to_server("NOTICE %s :\001ERRMSG Unknown dcc type: %s.\001\n",getnick(orig),cmd);
	}	
}

/* Function: Preprocess an incoming file. */
void open_incoming_file(char *orig,char *args)
{
	struct user_t *tmp;
	struct chat_t *ctmp;
	char *addr,*port,*size,*file,*efile=NULL,path[NAME_MAX*2+1];
	unsigned long laddr,lsize;
	unsigned int iport;

	if (!(tmp=get_alias(getnick(orig),&users)))
	{
		send_to_server("NOTICE %s :\001ERRMSG I do not accept files from strangers.\001\n",getnick(orig));
		return;
	}
	
	if (!(ctmp=find_chat(tmp,&chatlist)))
	{
		send_to_server("NOTICE %s :\001ERRMSG I am sorry, I cannot accept a file unless you're connected.\001\n",getnick(orig));
		return;
	}

	if (!(file=strtok(args," "))) return; /* This is a fake DCC */
	if (!(addr=strtok(NULL," "))) return; /* Fake */
	if (!(port=strtok(NULL," "))) return; /* Fake */
	if (!(size=strtok(NULL," "))) return; /* Fake */
	sscanf(addr,"%lu",&laddr);
	sscanf(port,"%u",&iport);
	sscanf(size,"%lu",&lsize);

	if (!laddr||iport<=1024||!lsize) return; /* Hmmm, suspicous */
	
	sprintf(path,"%s%s/%s",cfg.filedir,ctmp->chatter->dir,file);
	if (!(efile=enquote(path)))
	{
		DEBUG1("open_incoming_file(): enquote(\"%s\") returned NULL.\n",path);
		return;
	}
	send_to_process(ctmp,GET,"%s %lu %u %lu",efile,laddr,iport,lsize);
	log(DCC,"Receiving %s%s%s from %s.",cfg.filedir,ctmp->chatter->dir,file,ctmp->chatter->nick);
	free(efile);
}	

/* Function: Sets up a DCC SEND connection. */
void process_outgoing_file(char **mv,int dcctype)
{
#if ENABLE_TAR
	char *tmpch,tempstr[NAME_MAX+1],command[NAME_MAX+1];
#endif
	struct dcc_t *tmp;
	struct stat statbuf;

	if (dcc_exists(mv[1]))
	{
#if ENABLE_TAR
		send_to_process(NULL,LIST,"%s \002%s%s\002 is already being DCCed.",mv[0],base_name(mv[1]),dcctype&D_TAR?".tar":"");
#else
		send_to_process(NULL,LIST,"%s \002%s\002 is already being DCCed.",mv[0],base_name(mv[1]));
#endif
		return;
	}

	if (!(tmp=(struct dcc_t *)malloc(sizeof(struct dcc_t))))
	{
		/* No mem */
		return;
	}	

	memset(tmp,0,sizeof(struct dcc_t));
	strcpy(tmp->fname,mv[1]);
	tmp->type=dcctype;
	tmp->active=0;
	tmp->transmit=0;
	tmp->bsize=cfg.dccblocksize;

	if (dcctype&D_SEND)
	{
		if (!(tmp->file=fopen(tmp->fname,"r")))
		{
			/* Cannot open file. Bummer. */
			free(tmp);
			return;
		}
	
		if (stat(tmp->fname,&statbuf)==-1)
		{
			/* I can't really imagine how this could happen... */
			fclose(tmp->file);
			free(tmp);
			return;
		}

		if (!(tmp->total=(unsigned long)statbuf.st_size))
		{
			fclose(tmp->file);
			free(tmp);
			return;
		}
#ifdef ENABLE_RATIOS
		/* Do we have enough credits left for this? */
		if (ratio>0&&cred<tmp->total)
		{
			send_to_process(NULL,LIST,"? You need \002%llu\002 more credits to receive \002%s\002.",tmp->total-cred,base_name(tmp->fname));
			fclose(tmp->file);
			free(tmp);
			return;
		}
#endif
	}
#if ENABLE_TAR
	else if (dcctype&D_TAR)
	{
		strcpy(tempstr,mv[1]);
		if (!(tmpch=strrchr(tempstr,(int)'/')))
		{
			free(tmp);
			return;
		}	
		*tmpch='\0';
		if (chdir(tempstr))
		{
			free(tmp);
			return;
		}	

		/* 
		 * It's important to predict the correct size of the tar
		 * archive, because a lot of irc clients chops off the DCC
		 * when the given number of bytes has been received. I
		 * think this works, but it's possible that some tar bins
		 * use a different default block alignment (normal is 20*512).
		 */
		if (!(tmp->total=calculate_tarsize(mv[1])))
		{
			free(tmp);
			return;
		}
		tmp->total+=1024; /* The size of the 2 end marker blocks. */
		tmp->total+=(unsigned long)((tmp->total%10240)?(10240-(tmp->total%10240)):0);

#ifdef ENABLE_RATIOS
		if (ratio>0&&cred<tmp->total)
		{
			send_to_process(NULL,LIST,"? You need \002%llu\002 more credits to receive \002%s.tar\002.",tmp->total-cred,base_name(tmp->fname));
			free(tmp);
			return;
		}
#endif /* ENABLE_RATIOS */
		/* Create the tar commad and redirect errors to /dev/null. */
		sprintf(command,"%s cf - '%s' 2>/dev/null",TAR_PATH,base_name(mv[1]));
		if (!(tmp->file=popen(command,"r")))
		{
			/* Cannot open file. Bummer. */
			free(tmp);
			return;
		}
	}	
#endif /* ENABLE_TAR */

	if ((tmp->tmpfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		if (dcctype&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
		else if (dcctype&D_TAR) pclose(tmp->file);
#endif
		free(tmp);
		return;
	}

	memset((char *) &tmp->srv_addr,0,sizeof(tmp->srv_addr));
	tmp->srv_addr.sin_family=AF_INET;
	tmp->srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	tmp->port=next_port();
	tmp->srv_addr.sin_port=htons(tmp->port);

	while (bind(tmp->tmpfd,(struct sockaddr *)&tmp->srv_addr,sizeof(tmp->srv_addr))<0)
	{
		if (errno!=EADDRINUSE)
		{
			/* Damnit, can't bind local address... */
			if (dcctype&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
			else if (dcctype&D_TAR) pclose(tmp->file);
#endif
			close(tmp->tmpfd);
			free(tmp);
			return;
		}
		else
		{
			tmp->port=next_port();
			tmp->srv_addr.sin_port=htons(tmp->port);
		}
	}

	if (listen(tmp->tmpfd,4))
	{
		if (dcctype&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
		else if (dcctype&D_TAR) pclose(tmp->file);
#endif
		close(tmp->tmpfd);
		free(tmp);
		return;
	}

	if (fcntl(tmp->tmpfd,F_SETFL,O_NONBLOCK)<0)
	{
		if (dcctype&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
		else if (dcctype&D_TAR) pclose(tmp->file);
#endif
		close(tmp->tmpfd);
		free(tmp);
		return;
	}

	time(&tmp->stime);
	tmp->next=dcclist;
	dcclist=tmp;

#ifdef ENABLE_RATIOS
	/* This is done to avoid "double booked" credits. */
	if (ratio>0)
	{
		cred-=tmp->total;
		send_to_process(NULL,RATIO,"CRED %llu",cred);
	}
#endif

	/* Report the number of open DCCs. */
	send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());

	/* Report back the DCC SEND offer. */
#if ENABLE_TAR
	send_to_process(NULL,SEND,"%s %s%s %lu %u %lu",mv[0],safe_name(base_name(tmp->fname)),tmp->type&D_TAR?".tar":"",local_ip,tmp->port,tmp->total);
#else
	send_to_process(NULL,SEND,"%s %s %lu %u %lu",mv[0],safe_name(base_name(tmp->fname)),local_ip,tmp->port,tmp->total);
#endif
}


/* Function: Sets up a DCC GET connection. */
void process_incoming_file(char **mv)
{
	unsigned long addr,size;
	int port;
	char *openflag;
	struct dcc_t *tmp;

	sscanf(mv[1],"%lu",&addr);
	sscanf(mv[2],"%u",&port);
	sscanf(mv[3],"%lu",&size);

	if (dcc_exists(mv[0]))
	{
		send_to_process(NULL,LIST,"? \002%s\002 is already being DCCed. File skipped.",mv[0]);
		return;
	}

	if (!(tmp=(struct dcc_t *)malloc(sizeof(struct dcc_t))))
	{
		return;
	}

	/* Initialize the dcc structure. */
	memset(tmp,0,sizeof(struct dcc_t));
	strcpy(tmp->fname,mv[0]);
	tmp->type=D_GET;
	tmp->active=1; /* Well, these are always active, I suppose. */
	tmp->total=size;
	tmp->transmit=0;
	tmp->bsize=0;
	
	memset((char *)&tmp->srv_addr,0,sizeof(tmp->srv_addr));
	
	tmp->srv_addr.sin_family=AF_INET;
	tmp->srv_addr.sin_addr.s_addr=htonl(addr);
	tmp->srv_addr.sin_port=htons(port);

	if ((tmp->sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		free(tmp);
		return;
	}


	/* Open the file... */
	if (cfg.overwrite) openflag="w";
	else openflag="w+";
	
	if (!(tmp->file=fopen(tmp->fname,openflag)))
	{
		close(tmp->sockfd);
		free(tmp);
		return;
	}

	if (fcntl(tmp->sockfd,F_SETFL,O_NONBLOCK)==-1)
	{
		close(tmp->sockfd);
		fclose(tmp->file);
		free(tmp);
		return;
	}

	/* Set up the connection */	
	if (connect(tmp->sockfd,(struct sockaddr *)&tmp->srv_addr,sizeof(tmp->srv_addr))<0)
	{
		if (errno!=EINPROGRESS)
		{
			close(tmp->sockfd);
			fclose(tmp->file);
			free(tmp);
			return;
		}
		else
		{
			tmp->active=0;
		}
	}

	/* Insert this DCC into the list */
	tmp->next=dcclist;
	dcclist=tmp;

	/* Report the number of open DCCs. */
	send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());
}

/* Function: Returns the number of open DCCs. */
int dcc_count()
{
	struct dcc_t *tmp;
	int count;

	for (tmp=dcclist,count=0;tmp;tmp=tmp->next,count++);
	return(count);
}


/* Function: Returns the number of open DCCs. */
int queue_count()
{
	struct dccqueue_t *tmp;
	int count;

	for (tmp=dccqueue,count=0;tmp;tmp=tmp->next,count++);
	return(count);
}

/* Function: Puts a DCC on deathrow for removal. */
void put_on_deathrow(struct dcc_t *convict)
{
	struct deathrow_t *tmp;
	
	if ((tmp=(struct deathrow_t *)malloc(sizeof(struct deathrow_t))))
	{
		memset(tmp,0,sizeof(struct deathrow_t));
		tmp->convict=convict;
		tmp->next=deathrow;
		deathrow=tmp;
	}
	
	/* There ain't much we can do if we can't allocate mem */
}

/* Function: Removes any DCCs on deathrow. */
void clear_deathrow()
{
	struct dcc_t *tmp,*prev;
	struct deathrow_t *conv;
	
	if (deathrow)
	{
		while (deathrow)
		{
			tmp=deathrow->convict;
			if (tmp==dcclist)
			{
				dcclist=tmp->next;
				free(tmp);
			}
			else
			{
				prev=dcclist;
				tmp=prev->next;
				while ((tmp!=deathrow->convict)&&(tmp))
				{
					prev=tmp;
					tmp=tmp->next;
				}
				if (tmp)
				{
					prev->next=tmp->next;
					free(tmp);
				}
			}
			conv=deathrow;
			deathrow=conv->next;
			free(conv);
		}		
		/* Report the number of open DCCs. */
		send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());
	}
}

/* Function: Checks all dccs and do whatever necessary. */
void check_dcc()
{
	fd_set readset;
	struct timeval tval={0,50};
	struct dcc_t *tmp;
	int biggest=0;

	FD_ZERO(&readset);

	for (tmp=dcclist;tmp;tmp=tmp->next)
	{
		if (tmp->active)
		{
			if (tmp->sockfd>biggest) biggest=tmp->sockfd;
			FD_SET(tmp->sockfd,&readset);
		}
	}


	select(biggest+1,&readset,(fd_set *)0,(fd_set *)0,&tval);

	for (tmp=dcclist;tmp;tmp=tmp->next)
	{
		if ((!tmp->active)||FD_ISSET(tmp->sockfd,&readset))
		{
			switch(tmp->type&(~D_RESUME))
			{
			case D_GET:
				process_dccget(tmp);
				break;
			case D_SEND:
#if ENABLE_TAR
			case D_TAR:
#endif
				process_dccsend(tmp);
				break;
			default:
				break;
			}
		}
	}
	/* Let's get rid of some dead weight. */
	check_dcc_timeout();
	clear_deathrow();
	/* Maybe there are some DCCs in the queue. */
	check_dccqueue();
}

/* Function: Reads next chunk of data from a DCC GET, and writes to file. */
void process_dccget(struct dcc_t *tmp)
{
	unsigned long bytesread=0;
	unsigned long bytestmp;
	char tmpbuff[BIG_BUFFER_SIZE+1];
		
	if (tmp->active)
	{
		if ((bytesread=read(tmp->sockfd,tmpbuff,BIG_BUFFER_SIZE))<=0)
		{
#ifdef ENABLE_RATIOS
			if (tmp->total==tmp->transmit)
			{
				ul+=tmp->total;
				cred+=tmp->total*ratio;
				send_to_process(NULL,RATIO,"UL %llu",ul);
				send_to_process(NULL,RATIO,"CRED %llu",cred);
			}
#endif
			fclose(tmp->file);
			close(tmp->sockfd);
			put_on_deathrow(tmp);
		}
		fwrite(tmpbuff,1,bytesread,tmp->file);
		tmp->transmit+=bytesread;
		bytestmp=htonl(tmp->transmit);
		write(tmp->sockfd,&bytestmp,sizeof(bytestmp));
	}
	else if (connect(tmp->sockfd,(struct sockaddr *)&tmp->srv_addr,sizeof(tmp->srv_addr))<0&&errno!=EALREADY)
	{
		if (errno!=EINPROGRESS)
		{
			close(tmp->sockfd);
			fclose(tmp->file);
			put_on_deathrow(tmp);
			return;
		}
	}
	else
	{
		tmp->active=1;
		time(&tmp->stime);
	}
}


/* Function: Attends to an open DCC SEND and does what's needed. */
void process_dccsend(struct dcc_t *tmp)
{
	unsigned long ack=0;
	int bytesread=0,maxread,toread;
	char buf[tmp->bsize];
	
	if (!tmp->active)
	{
		if ((tmp->sockfd=accept(tmp->tmpfd,(struct sockaddr *) &tmp->cli_addr,&tmp->clilen))==-1)
		{
			if (errno!=EWOULDBLOCK&&errno!=EAGAIN)
			{
#ifdef ENABLE_RATIOS
				if (ratio>0)
				{
					cred+=tmp->type&D_RESUME?tmp->resume:tmp->total;
					send_to_process(NULL,RATIO,"CRED %llu",cred);
				}
#endif
				if (tmp->type&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
				else if (tmp->type&D_TAR) pclose(tmp->file);
#endif
				close(tmp->tmpfd);
				put_on_deathrow(tmp);
			}
			return;						
		}
		fcntl(tmp->sockfd,F_SETFL,O_NONBLOCK);
		tmp->active=1;
		if (tmp->type&D_RESUME) ack=tmp->transmit;
		close(tmp->tmpfd);
		/* Reset start time for measurement purposes. */
		time(&tmp->stime);
	}
	else 
	if ((read(tmp->sockfd,&ack,sizeof(ack))<sizeof(ack))||(tmp->type&D_RESUME)?(ntohl(ack)==tmp->resume):(ntohl(ack)==tmp->total))
	{
#ifdef ENABLE_RATIOS
		if (tmp->type&D_RESUME?ntohl(ack)==tmp->resume:ntohl(ack)==tmp->total)
		{
			dl+=ntohl(ack);
			send_to_process(NULL,RATIO,"DL %llu",dl);
		}
		else if (ratio>0)
		{
			cred+=tmp->type&D_RESUME?tmp->resume:tmp->total;
			send_to_process(NULL,RATIO,"CRED %llu",cred);
		}
#endif
		close(tmp->sockfd);
		if (tmp->type&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
		else if (tmp->type&D_TAR) fclose(tmp->file);
#endif
		put_on_deathrow(tmp);
		return;
	}

	if (tmp->total==tmp->transmit) return;

	maxread=(tmp->total-tmp->transmit)<tmp->bsize?tmp->total-tmp->transmit:tmp->bsize;
	toread=maxread-(tmp->transmit-ntohl(ack));

	bytesread=fread(buf,1,toread,tmp->file);
	tmp->transmit+=bytesread;
	write(tmp->sockfd,buf,bytesread);

}

/* Function: Sends a DCC offer to a client. */
void send_dcc_offer(char *dccinfo)
{
	char *dest;

	for (dest=dccinfo;*dccinfo!=' ';dccinfo++);
	*dccinfo++='\0';
	send_to_server("PRIVMSG %s :\001DCC SEND %s\001\n",dest,dccinfo);
}

/* Function: Checks for timed out DCCs and if necessary, put's them to death. */
void check_dcc_timeout()
{
	struct dcc_t *tmp;
	
	for (tmp=dcclist;tmp;tmp=tmp->next)
	{
		if ((!tmp->active)&&(time(NULL)>(tmp->stime+cfg.dcctimeout)))
		{
			close(tmp->tmpfd);
			if (tmp->type&D_SEND) fclose(tmp->file);
#if ENABLE_TAR
			else if (tmp->type&D_TAR) pclose(tmp->file);
#endif
			put_on_deathrow(tmp);
		}
	}
}

/* Function: Puts a file on the DCC queue. */
void put_on_dccqueue(int type,char **mv)
{
	struct dccqueue_t *tmp,*p;
	char *word,*dest;

	dest=type&D_GET?"?":mv[0];
	word=type&D_GET?mv[0]:mv[1];

	if (is_in_queue(word)||dcc_exists(word))
	{
#if ENABLE_TAR
		send_to_process(NULL,LIST,"%s \002%s%s\002 is already being DCCed or is in queue.",dest,base_name(word),(type==D_TAR)?".tar":"");
#else
		send_to_process(NULL,LIST,"%s \002%s\002 is already being DCCed or is in queue.",dest,base_name(word));
#endif
		return;
	}

	if (!(tmp=(struct dccqueue_t *)malloc(sizeof(struct dccqueue_t)))) return;
	memset(tmp,0,sizeof(struct dccqueue_t));
	tmp->type=type;
	type&D_GET?sprintf(tmp->info,"%s",mv[0]):sprintf(tmp->info,"%s %s",mv[0],mv[1]);
	tmp->next=NULL;
	
	if (!dccqueue)
	{
		dccqueue=tmp;
	}
	else
	{
		for (p=dccqueue;p->next;p=p->next);
		p->next=tmp;
	}
	/* Report the number of open DCCs. */
	send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());
#if ENABLE_TAR
	send_to_process(NULL,LIST,"%s \002%s%s\002 has been put on DCC queue.",dest,base_name(word),(type&D_TAR)?".tar":"");
#else
	send_to_process(NULL,LIST,"%s \002%s\002 has been put on DCC queue.",dest,base_name(word));
#endif
}

/* Function: Checks the DCC queue for DCCs to be processed. */
void check_dccqueue()
{
	struct dccqueue_t *tmp;
	static int counter;
	char **av;

	/* It's a waste of time checking this too often */
	if ((!autodcc)||(counter++%50)||(!dccqueue)) return;
	
	for (tmp=dccqueue;(tmp)&&(dcc_count()<dcclimit);tmp=dccqueue)
	{
		switch(tmp->type)
		{
		case D_GET:
			if (!(av=parse_line(tmp->info))) break;
			process_incoming_file(av);
			free_argv(av);
			break;
		case D_SEND:
#if ENABLE_TAR
		case D_TAR:
#endif
			if (!(av=parse_line(tmp->info))) break;
			process_outgoing_file(av,tmp->type);
			free_argv(av);
			break;
		default:
			break;
		}
		dccqueue=tmp->next;
		free(tmp);
		send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());
	}
}

/* Functions: Displays a list of the files being DCCed. */
void show_dcc_list(char **mv)
{
	int index,etas;
	struct dcc_t *tmp;
	char typetxt[10];
	char speed[10];
	char eta[10];
	char stat[10];
	
	if (!dcclist)
	{
		send_to_process(NULL,LIST,"%s No open DCCs.",mv[0]);
		return;
	}
	else
	{
		send_to_process(NULL,LIST,"%s No. Type      Size  Prc.    K/s   ETA  Stat  Name",mv[0]);
	}

	for (tmp=dcclist,index=1;tmp;tmp=tmp->next,index++)
	{
		switch(tmp->type&(~D_RESUME))
		{
		case D_GET:
			strcpy(typetxt,"GET");
			break;
		case D_SEND:
			strcpy(typetxt,"SEND");
			break;
#if ENABLE_TAR
		case D_TAR:
			strcpy(typetxt,"TAR");
			break;
#endif
		default:
			strcpy(typetxt,"???");
			break;
		}
		sprintf(stat,tmp->active?"Actv":"Wait");
		if (!tmp->transmit)
		{
			sprintf(speed,"  N/A ");
			sprintf(eta," N/A ");
		}
		else	
		{
			sprintf(speed,"%3.2f",((double)tmp->transmit)/(double)(time(NULL)-(tmp->stime))/1024);
			etas=(double)((tmp->type&D_RESUME?tmp->resume:tmp->total)-tmp->transmit)*(double)(time(NULL)-(tmp->stime))/(unsigned long long)tmp->transmit;
			if (etas>=6000) sprintf(eta,"%.4dm",etas/60);
			else sprintf(eta,"%.2d:%.2d",etas/60,etas%60);
		}
#if ENABLE_TAR
		send_to_process(NULL,LIST,"%s #%-2d %-4s %9lu  %3d%c  %-6s %-5s %s  \002%s%s\002",
			mv[0],index,typetxt,tmp->total,
			(int)(((unsigned long long)100*(unsigned long long)tmp->transmit)/(unsigned long long)(tmp->type&D_RESUME?tmp->resume:tmp->total)),
			'%',speed,eta,stat,base_name(tmp->fname),tmp->type&D_TAR?".tar":"");
#else
		send_to_process(NULL,LIST,"%s #%-2d %-4s %9lu  %3d%c  %-6s %-5s %s  \002%s\002",
			mv[0],index,typetxt,tmp->total,
			(int)(((unsigned long long)100*(unsigned long long)tmp->transmit)/(unsigned long long)(tmp->type&D_RESUME?tmp->resume:tmp->total)),
			'%',speed,eta,stat,base_name(tmp->fname));
#endif
	}
	send_to_process(NULL,LIST,"%s End of DCC listing.",mv[0]);
}

/* Function: Checks to see if a file is being DCCed. */
int dcc_exists(char *filename)
{
	struct dcc_t *p;

	for (p=dcclist;(p)&&(strcmp(p->fname,filename));p=p->next);
	if (p) return(1);
	return(0);
}

/* Function: Checks to see if a file is in the DCC queue. */
int is_in_queue(char *filename)
{
	struct dccqueue_t *p;
	char sometext[256],*word;

	for (p=dccqueue;p;p=p->next)
	{
		strcpy(sometext,p->info);
		word=strtok(sometext," ");
		if (p->type&D_GET&&!strcmp(word,filename))
		{
			return(1);
		}
		else if (!strcmp(strtok(NULL," "),filename))
		{
			return(1);
		}
	}
	return(0);
}

/* Function: Displays a list of the files in the DCC queue. */
void show_dcc_queue(char **mv)
{
	int idx;
	struct dccqueue_t *dq;
	char typetxt[10],sometext[256],*first;

	if (!dccqueue)
	{
		send_to_process(NULL,QLIST,"%s No files enqueued.",mv[0]);
	}
	else
	{
		send_to_process(NULL,QLIST,"%s No. Type   Name",mv[0]);
		for (idx=1,dq=dccqueue;dq;dq=dq->next,idx++)
		{
			switch (dq->type)
			{
			case D_GET:
				strcpy(typetxt,"GET");
				break;
			case D_SEND:
				strcpy(typetxt,"SEND");
				break;
#if ENABLE_TAR
			case D_TAR:
				strcpy(typetxt,"TAR");
				break;
#endif
			default:
				strcpy(typetxt,"???");
				break;
			}
			strcpy(sometext,dq->info);
			first=strtok(sometext," ");
#if ENABLE_TAR
			send_to_process(NULL,QLIST,"%s #%-2d %-5s \002%s%s\002",mv[0],idx,typetxt,base_name(dq->type&D_GET?first:strtok(NULL," ")),(dq->type==D_TAR)?".tar":"");
#else
			send_to_process(NULL,QLIST,"%s #%-2d %-5s \002%s\002",mv[0],idx,typetxt,base_name(dq->type&D_GET?first:strtok(NULL," ")));
#endif
		}
		send_to_process(NULL,QLIST,"%s End of list.",mv[0]);
	}
}


/* Function: Removes DCCs from the DCC list. */
void remove_dcc(char **mv)
{
	char *dest,*num;
	int inum,idx;
	struct dcc_t *p;

	dest=mv[0];
	num=mv[1];
	
	if (strcasecmp(num++,"#all"))
	{
		sscanf(num,"%d",&inum);
	
		for (idx=1,p=dcclist;(p)&&(idx!=inum);p=p->next,idx++);
		if ((p)&&(idx==inum))
		{
			switch(p->type&(~D_RESUME))
			{
#if ENABLE_TAR
			case D_TAR:
#endif
			case D_SEND:
#if ENABLE_TAR
				send_to_process(NULL,LIST,"%s Closed DCC SEND \002%s%s\002 at \002%lu\002 bytes.",dest,base_name(p->fname),p->type&D_TAR?".tar":"",p->transmit);
#else
				send_to_process(NULL,LIST,"%s Closed DCC SEND \002%s\002 at \002%lu\002 bytes.",dest,base_name(p->fname),p->transmit);
#endif
				if (!p->active) close(p->tmpfd);
				else close(p->sockfd);
				if (p->type&D_SEND) fclose(p->file);
#if ENABLE_TAR
				else if (p->type&D_TAR) pclose(p->file);
#endif
				break;
			case D_GET:
				send_to_process(NULL,LIST,"%s Closed DCC GET \002%s\002 at \002%lu\002 bytes.",dest,base_name(p->fname),p->transmit);
				close(p->sockfd);
				fclose(p->file);
				break;
			default:
				send_to_process(NULL,LIST,"%s Closed DCC ??? \002%s\002 at \002%lu\002 bytes.",dest,p->fname,p->transmit);
				break;
			}
#ifdef ENABLE_RATIOS
			if (p->type&D_RESUME?p->transmit<p->resume:p->transmit<p->total&&ratio>0)
			{
				cred+=p->type&D_RESUME?p->resume:p->total;
				send_to_process(NULL,RATIO,"CRED %llu",cred);
			}
#endif
			put_on_deathrow(p);
		}
	}
	else
	{
		for (p=dcclist;p;p=p->next)
		{
			switch(p->type&(~D_RESUME))
			{
#if ENABLE_TAR
			case D_TAR:
#endif
			case D_SEND:
				if (!p->active) close(p->tmpfd);
				else close(p->sockfd);
				if (p->type&D_SEND) fclose(p->file);
#if ENABLE_TAR
				else if (p->type&D_TAR) pclose(p->file);
#endif
				break;
			case D_GET:
				close(p->sockfd);
				fclose(p->file);
				break;
			default:
				break;
			}
#ifdef ENABLE_RATIOS
			if (p->type&D_RESUME?p->transmit<p->resume:p->transmit<p->total&&ratio>0)
			{
				cred+=p->type&D_RESUME?p->resume:p->total;
				send_to_process(NULL,RATIO,"CRED %llu",cred);
			}
#endif
			put_on_deathrow(p);
			clear_deathrow();
		}
		send_to_process(NULL,LIST,"%s All DCCs closed.",dest);
	}
	clear_deathrow();
}

/* Function: Removes files from the DCC queue. */
void remove_queued(char **mv)
{
	int num,idx;
	char *dest,*arg;
	struct dccqueue_t *p,*q;
	
	dest=mv[0];
	arg=mv[1];
	
	if (strcasecmp(arg++,"#all"))
	{

		/* One file */
		if (!(num=atoi(arg)))
		{
			send_to_process(NULL,LIST,"%s Unable to remove item.",dest);
			return;
		}
		if (!queue_count())
		{
			send_to_process(NULL,LIST,"%s Queue is already empty.",dest);
			return;
		}		
		if (num==1)
		{
			p=dccqueue;
			dccqueue=p->next;
			free(p);
			send_to_process(NULL,LIST,"%s Item \002%d\002 has been dequeued.",dest,num);
		}
		else
		{
			for (idx=2,q=dccqueue,p=q->next;(p)&&(idx<num);q=p,p=p->next,idx++);
			if (p)
			{
				q->next=p->next;
				free(p);
				send_to_process(NULL,LIST,"%s Item \002%d\002 has been dequeued.",dest,num);
			}
			else
			{
				send_to_process(NULL,LIST,"%s Unable to remove item.",dest);
			}
		}
	}
	else
	{
		/* All files */
		for (p=dccqueue;p;p=dccqueue)
		{
			dccqueue=p->next;
			free(p);
		}
		send_to_process(NULL,LIST,"%s All files removed from queue.",dest);
	}
	send_to_process(NULL,NUM,"%d %d",dcc_count(),queue_count());
}


/* Function: Pulls files from the dcc queue. */
void process_next(char **mv)
{
	int cnt,diff,idx;
	struct dccqueue_t *tmp;
	char **av;
	
	sscanf(mv[0],"%d",&cnt);
	diff=dcclimit-dcc_count();
	if (diff<1)
	{
		send_to_process(NULL,NEXT,"? No files pulled from DCC queue.");
		return;
	}
	if (cnt>diff) cnt=diff;
	for (tmp=dccqueue,idx=cnt;(tmp)&&(idx>0);tmp=dccqueue,idx--)
	{
		switch(tmp->type)
		{
		case D_GET:
			if (!(av=parse_line(tmp->info))) break;
			process_incoming_file(av);
			free_argv(av);
			break;
		case D_SEND:
#if ENABLE_TAR
		case D_TAR:
#endif
			if (!(av=parse_line(tmp->info))) break;
			process_outgoing_file(av,tmp->type);
			free_argv(av);
			break;
		default:
			break;
		}
		dccqueue=tmp->next;
		free(tmp);
	}
	send_to_process(NULL,NEXT,"? \002%d\002 file%s pulled from DCC queue.",cnt-idx,(cnt-idx)>1?"s":"");
}

/* Function: Parses a mIRC Resume request, and notifies child. */
void parse_resume_request(char *orig,char *args)
{
	struct user_t *tmp;
	struct chat_t *ctmp;
	char filename[100];
	int port;
	unsigned long pos;

	if ((!(tmp=get_user(getnick(orig),&users)))||(!(ctmp=find_chat(tmp,&chatlist))))
	{
		send_to_server("NOTICE %s :\001ERRMSG Bogus resume request detected.\001\n",getnick(orig));
		return;
	}
	sscanf(args,"%s %d %lu",filename,&port,&pos);
	send_to_process(ctmp,RESUME,"%s %d %lu",filename,port,pos);
}

/* Function: Searches for resumable DCC and sends out acknowledgement. */
void process_resume(char **mv)
{
#if ENABLE_TAR
	char tarbuf[10240];
	unsigned long bytes2read;
#endif
	struct dcc_t *p;
	int port;
	unsigned long pos;
	
	sscanf(mv[1],"%d",&port);
	sscanf(mv[2],"%lu",&pos);
	
	for (p=dcclist;(p)&&p->port!=port;p=p->next);
	if ((p)&&!p->active&&pos<p->total)
	{
		p->resume=p->total;
		p->total-=pos;
		p->transmit=pos;
		if (p->type&D_SEND)
		{
			fseek(p->file,pos,SEEK_SET);
		}
#if ENABLE_TAR
		else if (p->type&D_TAR)
		{
			/* This is a clumsy & slow way to seek through a */
			/* tar file. If anyone got a better suggestion, */
			/* please tell me all about it. */
			do
			{	
				bytes2read=(pos<10240)?pos:10240;
				pos-=bytes2read;
				fread((void *)tarbuf,(size_t)1,(size_t)bytes2read,p->file);
			}
			while(pos);
		}
#endif
		send_to_process(NULL,RESUME,"%s %s %s",mv[0],mv[1],mv[2]);
		p->type|=D_RESUME;
		p->stime=time(NULL); /* Reset time to avoid premature removal. */
	}
}
