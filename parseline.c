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
 * $Id: parseline.c,v 1.4 1998/11/08 23:36:07 kluzz Exp $
 */


#include <janbot.h>


/*
 * This function takes a string and breaks it into a list of arguments, taking into
 * consideration the usage of single and double quotes, spaces and escape sequences
 * such as '\n', '\x0D' and '\013'.
 *
 * Input:
 *
 *   char *str  - String containing the line to parse.
 *
 * Returns:
 *
 *   Argument vector if okay, NULL if some error occurred (such as unbalanced quotes,
 *   or it was unable to allocate memory for the arguments).
 */

char **parse_line(char *str)
{
	int i,j,len,idx,escl,ac;
	char s[strlen(str)+1];
	char **av;
	
	len=strlen(str);
	/* A command line can never contain more than ((number of chars)/2 + 1)
	   elements. Add some more for the terminating NULL pointer. */
	ac=len/2+4; 
	if (!(av=(char **)malloc(sizeof(char *)*ac))) return(NULL);
	memset(s,0,sizeof(s));
	memset(av,0,sizeof(char *)*ac);

	for (i=j=idx=0;i<len;) /* For some reason I like for() better than while(), I don't know why. */
	{
		/* Check single quotes. */
		if (str[i]=='\x27')
		{
			/* Copy chars until '\x27' or end of string is encountered. */
			while ( (str[++i]!='\x27') && (i<len) )
			{
				s[j++]=str[i];
			}
			/* If end of string is encountered, the quotes are unbalanced. */
			if (i++>=len)
			{
				free_argv(av);
				return(NULL);
			}
		}

		/* Check double quotes. */
		else if (str[i]=='\x22')
		{
			/* Copy and unescape chars until '\x22' or end of string is encountered. */
			while ( (str[++i]!='\x22') && (i<len) )
			{
				/* A double quoted string may contain escape sequences we need to parse. */
				if (str[i]=='\\')
				{
					strcat(s,unescape(&str[i],&escl));
					i+=escl-1;
					j=strlen(s);
				}
				else s[j++]=str[i];
			}
			/* If end of string is encountered, the quotes are unbalanced. */
			if (i++>=len)
			{
				free_argv(av);
				return(NULL);
			}
		}

		/* Check spaces. */
		else if (str[i]==' ')
		{
			if (j>0)
			{
				s[j]='\0';
				if (idx<ac)	av[idx++]=strdup(s);
				j=0;
				memset(s,0,sizeof(s));
			}
			while ( (str[++i]==' ') && (i<len) );
		}

		/* Check for escape sequences. */
		else if (str[i]=='\\')
		{
			strcat(s,unescape(&str[i],&escl));
			i+=escl;
			j=strlen(s);
		}
		
		/* Normal characters are just flung into the destination string like manure. */
		else s[j++]=str[i++];
	}
	if ( (j>0) && (idx<ac) )
	{
		s[j]='\0';
		av[idx++]=strdup(s);
	}
	av[idx]=NULL;
	return(av);
}

/*
 * Decodes an escape sequence beginning with '\' to an unescaped string. If the
 * string contains an unknown escape sequence, such as '\z' or '\0F3', the escape
 * character plus the first succeeding character is return. The two examples mentioned
 * would return '\z' and '\0'. At function return, esclen contains the number of
 * characters from the input string that was used to decode the escape sequence.
 */
char *unescape(char *in,int *esclen)
{
	static char buf[16],wrk[5];
	char *s;
	int num;
	
	s=in+1;
	memset(buf,0,16);

	switch ((int)*s)
	{
	case 'a':
		strcpy(buf,"\a");
		*esclen=2;
		break;
	case 'b':
		strcpy(buf,"\e");
		*esclen=2;
		break;
	case 'e':
		strcpy(buf,"\e");
		*esclen=2;
		break;
	case 'f':
		strcpy(buf,"\f");
		*esclen=2;
		break;
	case 'n':
		strcpy(buf,"\n");
		*esclen=2;
		break;
	case 'r':
		strcpy(buf,"\r");
		*esclen=2;
		break;
	case 't':
		strcpy(buf,"\t");
		*esclen=2;
		break;
	case 'v':
		strcpy(buf,"\e");
		*esclen=2;
		break;
	case 0x22:
	case 0x27:
	case '\\':
	case ' ':
		strncpy(buf,s,1);
		*esclen=2;
		break;
	case 'X':
	case 'x':
		strcpy(wrk,"0x"); /* Decodes a hexadecimal number, such as '\x0D'. */
		strncat(wrk,s+1,2);
		if (sscanf(wrk,"%x",&num))
		{
			strcpy(buf," ");
			*buf=(char)num;
			*esclen=4;
		}
		else
		{
			strncpy(buf,in,2);
			*esclen=2;
		}
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		strncpy(wrk,s+1,3); /* Decodes a decimal number, such as '\123'. */
		if ((sscanf(wrk,"%3d",&num))&&(num<256)&&(num>=0))
		{
			*buf=(char)num;
			*esclen=4;
		}
		else
		{
			strncpy(buf,in,2);
			*esclen=2;
		}
		break;
	default:
		strncpy(buf,in,2);
		*esclen=2;
		break;
	}
	return(buf);
}


/* Gets rid of an allocated string vector. Always use this function to drop a string vector.*/
void free_argv(char **av)
{
	int i;
	for (i=0;av[i];i++) free(av[i]);
	free(av);
}

/* Returns a string containing an enquoted version of the input string. This must be
   freed by the calling function. Returns NULL if something went wrong. */
char *enquote(char *in)
{
	int i,len;
	char *r;
	
	/* Find the length of the destination string. */	
	for (i=0,len=0;in[i];i++)
	{
		switch((int)in[i])
		{
		case 0x22: case 0x27: case '\\': case ' ':
			len+=4;
			break;
		default:
			len++;
			break;
		}
	}
	
	if (!(r=(char *)malloc(len+1))) return(NULL);
	memset(r,0,len+1);
	
	/* Create the destination string. */	
	for (i=0,len=0;in[i];i++)
	{
		switch((int)in[i])
		{
		case 0x22: case 0x27: case '\\': case ' ':
			sprintf(&r[len],"\\x%2x",(int)in[i]);
			len+=4;
			break;
		default:
			r[len++]=in[i];
			break;
		}
	}
	return(r);
}
