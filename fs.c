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
 * $Id: fs.c,v 1.7 1998/11/22 01:05:12 kluzz Exp $
 */


#include <janbot.h>

/* Function: Returns the base filename of a path. */
char *base_name(char *path)
{
	static char base[NAME_MAX+1];
	char *tmp;
	if ((tmp=strrchr(path,'/'))) strcpy(base,tmp+1);
	else strcpy(base,path);
	return(base);
}

/* Make a filename safe for DCC transferring. The regular ircII client doesn't support sending
   of filenames containing spaces, so there's no precedence on how to handle them. Right now
   it's enough to change most special characters to '_'. */
char *safe_name(char *n)
{
	static char nn[NAME_MAX+1];
	int i;
	
	for (i=0;n[i];i++)
	{
		/* We use almost the same algorithm as for nick names. */
		if ( (n[i]<'A' || n[i]>'}') && (n[i]<'0' || n[i]>'9') && n[i]!='-' && n[i]!='.') nn[i]='_';
		else nn[i]=n[i];
	}
	nn[i]='\0';
	return(nn);
}

/* Function: Converts a mode_t to a string representation. Still broken according to Bourbon. */
char *mode2str(unsigned int md)
{
	static char str[11];

	str[0]='-';	
	str[1]=(md & S_IRUSR) ? 'r' : '-';
	str[2]=(md & S_IWUSR) ? 'w' : '-';
	str[3]=(md & S_IXUSR) ? 'x' : '-';
	str[4]=(md & S_IRGRP) ? 'r' : '-';
	str[5]=(md & S_IWGRP) ? 'w' : '-';
	str[6]=(md & S_IXGRP) ? 'x' : '-';
	str[7]=(md & S_IROTH) ? 'r' : '-';
	str[8]=(md & S_IWOTH) ? 'w' : '-';
	str[9]=(md & S_IXOTH) ? 'x' : '-';
	str[10]='\0';

	str[3]=(md & S_ISUID) ? 'S' : str[3];
	str[6]=(md & S_ISGID) ? 'S' : str[6];
	str[9]=(md & S_ISUID) ? 'T' : str[9];

	if (md & S_IFLNK) *str='l';
	if (md & S_IFREG) *str='-';
	if (md & S_IFDIR) *str='d';

/* Actually, we don't care about these, they are device dependant. */
/*
	if (md & S_IFCHR) *str='c';
	if (md & S_IFBLK) *str='b';
	if (md & S_IFIFO) *str='f';
	if (md & S_IFSOCK) *str='s';
*/	

	return(str);
}



/* Function: Creates or erases a sorted list of files in a directory. */
struct dirlist_t *create_dirlist(DIR *thedir)
{
	struct dirent *entry;
	static struct dirlist_t *dlist;
	struct dirlist_t *p,*tmp;

	/* If the argument is NULL, let's delete the list. */
	
	if (!thedir)
	{
		for (p=dlist;p;p=dlist)
		{
			dlist=p->next;
			free(p);
		}
	}
	else
	while ((entry=readdir(thedir)))
	{
		if ((*entry->d_name)!='.')
		{
			if (!(tmp=(struct dirlist_t *)malloc(sizeof(struct dirlist_t))))
			{
				break;
			}
			memset(tmp,0,sizeof(struct dirlist_t));
			strcpy(tmp->name,entry->d_name);
			
			if (!dlist)
			{
				tmp->next=NULL;
				dlist=tmp;
			}
			else
			if (!dlist->next)
			{
				if (strcmp(tmp->name,dlist->name)<0)
				{
					tmp->next=dlist;
					dlist=tmp;
				}
				else
				{
					tmp->next=NULL;
					dlist->next=tmp;
				}
			}
			else if (strcmp(tmp->name,dlist->name)<0)
			{
				tmp->next=dlist;
				dlist=tmp;
			}
			else
			{
				for (p=dlist;(p->next)&&(strcmp(tmp->name,p->next->name)>0);p=p->next);
				tmp->next=p->next;
				p->next=tmp;
			}
		}
	}
	return(dlist);
}

/* Function: Generate a recursed listing of a directory. */
struct file_t *create_reclist(char *dir)
{
	DIR *d;
	struct dirent *e;
	struct file_t *p=NULL,*q=NULL,*r;
	struct list_t *s=NULL,*t=NULL;
	struct stat st;
	char name[NAME_MAX+1];

	if (!stat(dir,&st)&&st.st_mode&S_IFREG)
	{
		if ((q=(struct file_t *)malloc(sizeof(struct file_t))))
		{
			q->date=st.st_mtime;
			q->size=st.st_size;
			strcpy(q->name,name);
			q->next=NULL;
		}
		return(q);
	}			
	if (!(d=opendir(dir)))
	{
		return(NULL);
	}
	readdir(d); /* Get rid of ".." */
	readdir(d); /* ...and "." */
	while ((e=readdir(d)))
	{
		if ((t=ldup(e->d_name)))
		{
			t->str=strdup(e->d_name);
			if (!s)
			{
				t->next=NULL;
			}
			else
			{
				t->next=s;
			}
			s=t;
		}
	}
	closedir(d);
	for (t=s;t;s=t,t=t->next,lfree(s))
	{
		sprintf(name,"%s/%s",dir,t->str);
		if (!stat(name,&st)&&st.st_mode&S_IFDIR&&(r=create_reclist(name)))
		{
			if (!p)
			{
				p=r;
			}
			else
			{
				for (q=p;q->next;q=q->next);
				q->next=r;
			}
		}
		else if ((q=(struct file_t *)malloc(sizeof(struct file_t))))
		{
			q->date=st.st_mtime;
			q->size=st.st_size;
			strcpy(q->name,name);
			q->next=NULL;
			if (!p)
			{
				p=q;
			}
			else
			{
				for (r=p;r->next;r=r->next);
				r->next=q;
			}
		}
	}
	return(p);
}

/* Function: Verifies the existance of a directory */
int check_dir(char *dirname)
{
	char newdir[256];
	DIR *thedir;
	
	strcpy(newdir,cfg.filedir);
	strcat(newdir,dirname);
	
	if ((thedir=opendir(newdir)))
	{
		closedir(thedir);
		return(0);
	}
	return(1);
}

/* Function: Compares two directory entries by date and if required, name. */
int dirlist_compare(void *void1,void *void2)
{
	struct dirlist_t **ent1,**ent2;
	ent1=(struct dirlist_t **)void1;
	ent2=(struct dirlist_t **)void2;
	if ((*ent1)->sbuf.st_mtime<(*ent2)->sbuf.st_mtime ||
	   ((*ent1)->sbuf.st_mtime==(*ent2)->sbuf.st_mtime &&
	    strcmp((*ent1)->name,(*ent2)->name)>0)) return(1);
	else return(-1);
}

/* Function: Calculates the storage size of a tar archive. */
#if ENABLE_TAR
unsigned long calculate_tarsize(char *dir)
{
	DIR *current;
	struct dirent *ent;
	struct stat sb;
	char newname[NAME_MAX+1];
	unsigned long total=512;

	if (!(current=opendir(dir)))
	{
		return(0);
	}

	while((ent=readdir(current)))
	{
		if (strcmp(ent->d_name,".")&&strcmp(ent->d_name,".."))
		{
			sprintf(newname,"%s/%s",dir,ent->d_name);
			lstat(newname,&sb); /* If it's broken, fuck it. */
			if (S_ISDIR(sb.st_mode))
			{
				total+=calculate_tarsize(newname);
				DEBUG1("Checked dir: %s\n",newname);
			}
			else if (S_ISLNK(sb.st_mode))
			{
				total+=512;
				DEBUG1("Checked link: %s\n",newname);
			}
			else
			{
				total+=((unsigned long)sb.st_size)+(unsigned long)((sb.st_size%512)?(1024-(sb.st_size%512)):512);
				DEBUG1("Checked file: %s\n",newname);
			}
		}
	}
	closedir(current);
	return(total);
}
#endif /* ENABLE_TAR */

/* Function: Returns the full path to the user's home directory. */
char *gethomepath()
{
	static char home[NAME_MAX+1];
	char *p;
	struct passwd *pw;
	
	if ((p=getenv("HOME")))
	{
		strcpy(home,p);
	}
	else if ((pw=getpwuid(getuid())))
	{
		strcpy(home,pw->pw_dir);
	}
	else
	{
		strcpy(home,"");
	}
	return(home);
}

/* Function: Removes redundant slashes in path strings. */
void purify_path(char *path)
{
	char tmp[NAME_MAX+1],*t,*u;

	if (!strcmp(path,"/")) return;
	t=path;
	*tmp='\0';
	while ((u=strtok(t,"/")))
	{
		strcat(tmp,"/");
		strcat(tmp,u);
		t=NULL;
	}
	t=tmp;
	if (*path!='/') t++;
	strcpy(path,t);
}

/* Function: Resolves '.' and '..' in paths. Note: Needs absolute paths to work! */
char *resolve_path(char *path)
{
	int i,j,isdir=0;
	static char rpath[NAME_MAX+1];
	char *t=path,*u,*l1[256],*l2[256];

	if (path[strlen(path)-1]=='/') isdir=1;

	for (i=0;(u=strtok(t,"/"));i++)
	{
		l1[i]=strdup(u);
		t=NULL;
	}

	l1[i]=NULL;

	for (i=0,j=0;l1[i];i++)
	{
		if (!strcmp(l1[i],"..")&&j) j--;
		else if (strcmp(l1[i],".")) l2[j++]=l1[i];
	}

	l2[j]=NULL;
	
	*rpath='\0';
	for (i=0;l2[i];i++)	sprintf(rpath,"%s/%s",rpath,l2[i]);
	for (i=0;l1[i];i++) free(l1[i]);

	if (isdir) strcat(rpath,"/");
	
	return(rpath);
}

/* Function: Retrieves file info and returns a string to be printed. */
char *print_file_stats(struct dirlist_t *fp,char *pname)
{
	static char info[NAME_MAX+1];
	
	sprintf(info,"%s %10lu %s \002%s\002",
			mode2str(fp->sbuf.st_mode),(unsigned long)fp->sbuf.st_size,
			convert_date(fp->sbuf.st_mtime),pname);
	return(info);
}

/* Function: Creates an object containing a filename and it's stats. */
struct dirlist_t *strfdup(char *f)
{
	struct dirlist_t *p;
	
	if (!(p=(struct dirlist_t *)malloc(sizeof(struct dirlist_t)))) return(NULL);
	
	memset(p,0,sizeof(struct dirlist_t));
	
	if (stat(f,&p->sbuf))
	{
		free(p);
		return(NULL);
	}
	strcpy(p->name,f);
	return(p);
}

/* Function: Compares two dirlist_t entries based on different types of sorting. */
int dlcmp(struct dirlist_t *s1,struct dirlist_t *s2,int sorttype)
{
	char *p=(char *)s1->name,*q=(char *)s2->name;
	return strcmp(p,q);
}


/* Function: Returns a human readable representation of the free space of a given device (by path). */
#ifdef HAVE_STATFS
char *show_free_space(char *path)
{
	static char result[16];
	struct statfs sbuf;
	double i=1024;
	
	if (statfs(path,&sbuf))
	{
		sprintf(result,"Unknown");
		return(result);
	}

	i/=sbuf.f_bsize;
	i*=sbuf.f_bavail;

	if (i>=(1024*1024*1024)) sprintf(result,"%.1f TB",i/(1024*1024*1024));
	else if (i>=(1024*1024)) sprintf(result,"%.1f GB",i/(1024*1024));
	else if (i>=(1024)) sprintf(result,"%.1f MB",i/(1024));
	else sprintf(result,"%.1f KB",i);

	return(result);
}	
#endif
