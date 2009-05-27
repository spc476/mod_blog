
/***********************************************
*
* Copyright 2001 by Sean Conner.  All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <unistd.h>

#include <cgi/memory.h>
#include <cgi/ddt.h>
#include <cgi/htmltok.h>
#include <cgi/buffer.h>
#include <cgi/clean.h>
#include <cgi/util.h>
#include <cgi/cgi.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "chunk.h"
#include "wbtum.h"
#include "globals.h"

#define max(a,b)	((a) > (b)) ? (a) : (b)

/*****************************************************************/

static int	   cgi_main			(Cgi,int,char **);
static int	   cli_main			(int,char **);
static int         generate_pages		(void);
static void	   BlogDatesInit		(void);

static int	   tumbler_page			(FILE *,Tumbler);
static void	   calculate_previous		(struct tm);
static void	   calculate_next		(struct tm);
static const char *mime_type			(char *);
static int	   display_file			(Tumbler);
static int	   primary_page			(FILE *,int,int,int);
static int	   rss_page			(FILE *,int,int,int);
static int	   tab_page			(FILE *,int,int,int);
static void	   display_error		(int);

static void	   generic_cb			(const char *,FILE *,void *);
static void	   cb_blog_url			(FILE *,void *);
static void	   cb_blog_locallink		(FILE *,void *);
static void	   cb_blog_body			(FILE *,void *);
static void	   cb_blog_entry_locallinks	(FILE *,void *);
static void	   cb_blog_date			(FILE *,void *);
static void	   cb_blog_fancy		(FILE *,void *);
static void	   cb_blog_normal		(FILE *,void *);
static void	   cb_blog_name			(FILE *,void *);

static void	   cb_entry			(FILE *,void *);
static void	   cb_entry_id			(FILE *,void *);
static void	   cb_entry_url			(FILE *,void *);
static void	   cb_entry_date		(FILE *,void *);
static void	   cb_entry_number		(FILE *,void *);
static void	   cb_entry_title		(FILE *,void *);
static void	   cb_entry_class		(FILE *,void *);
static void	   cb_entry_author		(FILE *,void *);
static void	   cb_entry_body		(FILE *,void *);
static void	   cb_cond_hr			(FILE *,void *);

static void	   cb_rss_pubdate		(FILE *,void *);
static void	   cb_rss_url			(FILE *,void *);
static void	   cb_item			(FILE *,void *);
static void	   cb_item_url			(FILE *,void *);

static void	   cb_navigation_link		(FILE *,void *);
static void        cb_navigation_link_next	(FILE *,void *);
static void        cb_navigation_link_prev	(FILE *,void *);
static void	   cb_navigation_current	(FILE *,void *);
static void	   cb_navigation_current_url	(FILE *,void *);
static void	   cb_navigation_bar		(FILE *,void *);
static void	   cb_navigation_bar_next	(FILE *,void *);
static void	   cb_navigation_bar_prev	(FILE *,void *);
static void	   cb_navigation_next_url	(FILE *,void *);
static void	   cb_navigation_prev_url	(FILE *,void *);

static void	   cb_comments			(FILE *,void *);
static void	   cb_comments_body		(FILE *,void *);
static void	   cb_comments_filename		(FILE *,void *);
static void	   cb_comments_check		(FILE *,void *);

static void	   cb_begin_year		(FILE *,void *);
static void	   cb_now_year			(FILE *,void *);
static void        cb_update_time               (FILE *,void *);
static void        cb_update_type               (FILE *,void *);
static void	   cb_robots_index		(FILE *,void *);

#if 0
static void	   cb_calendar			(FILE *,void *);
static void	   cb_calendar_header		(FILE *,void *);
static void	   cb_calendar_days		(FILE *,void *);
static void	   cb_calendar_row		(FILE *,void *);
static void	   cb_calendar_item		(FILE *,void *);
#endif

static void	   print_nav_url		(FILE *,struct tm *,int);
static void	   fixup_uri			(BlogDay,HtmlToken,const char *);

/************************************************************************/

static struct chunk_callback  m_callbacks[] =
{
  { "blog.url"			, cb_blog_url			} ,
  { "blog.locallink"		, cb_blog_locallink		} ,
  { "blog.body"			, cb_blog_body			} ,
  { "blog.entry.locallinks"	, cb_blog_entry_locallinks	} ,
  { "blog.date"			, cb_blog_date			} ,
  { "blog.name"			, cb_blog_name			} ,
  { "blog.date.fancy"		, cb_blog_fancy			} ,
  { "blog.date.normal"		, cb_blog_normal		} ,

  { "entry"			, cb_entry			} ,
  { "entry.id"			, cb_entry_id			} ,
  { "entry.url"			, cb_entry_url			} ,
  { "entry.date"		, cb_entry_date			} ,
  { "entry.number"		, cb_entry_number		} ,
  { "entry.title"		, cb_entry_title		} ,
  { "entry.class"		, cb_entry_class		} ,
  { "entry.author"		, cb_entry_author		} ,
  { "entry.body"		, cb_entry_body			} ,
  
  { "comments"			, cb_comments			} ,
  { "comments.body"		, cb_comments_body		} ,
  { "comments.filename"		, cb_comments_filename		} ,
  { "comments.check"		, cb_comments_check		} ,
  
  { "cond.hr"			, cb_cond_hr			} ,

  { "rss.pubdate"		, cb_rss_pubdate		} ,
  { "rss.url"			, cb_rss_url			} ,
  { "item"			, cb_item			} ,
  { "item.url"			, cb_item_url			} ,
  
  { "navigation.link"		, cb_navigation_link		} ,
  { "navigation.link.next"	, cb_navigation_link_next	} ,
  { "navigation.link.prev"	, cb_navigation_link_prev	} ,
  { "navigation.bar"		, cb_navigation_bar		} ,
  { "navigation.bar.next"	, cb_navigation_bar_next	} ,
  { "navigation.bar.prev"	, cb_navigation_bar_prev	} ,
  { "navigation.next.url"	, cb_navigation_next_url	} ,
  { "navigation.prev.url"	, cb_navigation_prev_url        } ,
  { "navigation.current"	, cb_navigation_current		} ,
  { "navigation.current.url"	, cb_navigation_current_url	} ,

#if 0
  { "calendar"			, cb_calendar			} ,
  { "calendar.header"		, cb_calendar_header		} ,
  { "calendar.days"		, cb_calendar_days		} ,
  { "calendar.row"		, cb_calendar_row		} ,
  { "calendar.item"		, cb_calendar_item		} ,
#endif

  { "begin.year"		, cb_begin_year			} ,
  { "now.year"			, cb_now_year			} ,
  
  { "update.time"               , cb_update_time                } ,
  { "update.type"               , cb_update_type                } ,
  
  { "robots.index"		, cb_robots_index		} ,
  
  { NULL			, NULL				} 
};

#define CALLBACKS	((sizeof(m_callbacks) / sizeof(struct chunk_callback)) - 1)

/*************************************************************************/

static void generic_cb(const char *which,FILE *fpout,void *data)
{
  Chunk templates;
  
  ddt(which != NULL);
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  ChunkNew(&templates,g_templates,m_callbacks,CALLBACKS);
  ChunkProcess(templates,which,fpout,data);
  ChunkFree(templates);
}

/*********************************************************************/

static void cb_blog_url(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s/%s",g_baseurl,blog->date);
}

/*************************************************************************/

static void cb_blog_locallink(FILE *fpout,void *data)
{
  List    *plist = data;
  BlogDay  day;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);

  for (
        day = (BlogDay)ListGetHead(plist);
        NodeValid(&day->node);
        day = (BlogDay)NodeNext(&day->node)
      )
  {
    generic_cb("blog.locallink",fpout,day);
  }
}

/**********************************************************************/

static void cb_blog_body(FILE *fpout,void *data)
{
  List    *plist = data;
  BlogDay  day;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);

  if (m_htmldump)
  {
    while(1)
    {
      size_t s;
      char   buffer[BUFSIZ];
    
      s = fread(buffer,sizeof(char),BUFSIZ,m_htmldump);
      if (s == 0) break;
      fwrite(buffer,sizeof(char),s,fpout);
    }
    return;
  }

  for (
        day = (BlogDay)ListGetHead(plist);
        NodeValid(&day->node);
        day = (BlogDay)NodeNext(&day->node)
      )
  {
    generic_cb("blog.body",fpout,day);
  }
}

/*********************************************************************/

static void cb_blog_entry_locallinks(FILE *fpout,void *data)
{
  BlogDay blog = data;
  int            i;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_reverse)
  {
    for (i = blog->endentry ; i >= blog->stentry ; i--)
    {
      blog->curnum = i;
      generic_cb("blog.entry.locallinks",fpout,data);
    }
  }
  else
  {
    for (i = blog->stentry ; i <= blog->endentry ; i++)
    {
      blog->curnum = i; 
      generic_cb("blog.entry.locallinks",fpout,data);
    }
  }
}

/********************************************************************/

static void cb_blog_date(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",blog->date);
}

/********************************************************************/

static void cb_blog_name(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",g_name);
}

/*********************************************************************/

static void cb_blog_fancy(FILE *fpout,void *data)
{
  BlogDay    blog = data;
  struct tm  day;
  char      *p;
  char       dayname[BUFSIZ];
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  tm_init(&day);
  
  day.tm_year = strtoul(blog->date,&p,10) - 1900 ; p++;
  day.tm_mon  = strtoul(p,&p,10) - 1;              p++;
  day.tm_mday = strtoul(p,&p,10);
  
  mktime(&day);
  strftime(dayname,BUFSIZ,"%A",&day); /* all that just to get the day name */
  
  fprintf(fpout,"%s the %d<sup>%s</sup>",dayname,day.tm_mday,nth(day.tm_mday));
}

/************************************************************************/

static void cb_blog_normal(FILE *fpout,void *data)
{
  BlogDay    blog = data;
  struct tm  day;
  char       buffer[BUFSIZ];
  char      *p;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);

  tm_init(&day);

  day.tm_year = strtoul(blog->date,&p,10) - 1900 ; p++;
  day.tm_mon  = strtoul(p,&p,10) - 1;              p++;
  day.tm_mday = strtoul(p,&p,10);

  mktime(&day);
  
  strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&day);
  fprintf(fpout,"%s",buffer);
}

/***********************************************************************/

static void cb_entry(FILE *fpout,void *data)
{
  BlogDay blog = data;
  int            i;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);

  if (m_reverse)
  {
    for (i = blog->endentry ; i >= blog->stentry ; i--)
    {
      blog->curnum = i;
      BlogEntryRead(blog->entries[i]);
      generic_cb("entry",fpout,blog);
    }
  }
  else
  {
    for (i = blog->stentry ; i <= blog->endentry ; i++)
    {
      /*-------------------------------------------------------
      ; okay, blog->stentry can be -1 at this point because if
      ; certain conditions (:670-start.tm_hour can be 0), so, to
      ; avoid an error, I just simply continued, which doesn't print
      ; the entry if that's the only entry for the last day of the 
      ; blog, which is older than X days before.  So we revert 
      ; the fix, and instead, set i to 0.  Really gross hack, but
      ; until I get to the root cause, it will have to do. XXX
      ;--------------------------------------------------------*/

      /*if (i == -1) continue;*/ /* hack for quick fix */
      if (i == -1) i = 0;
      blog->curnum = i;
      BlogEntryRead(blog->entries[i]);
      generic_cb("entry",fpout,blog);
    }
  }
}

/**********************************************************************/

static void cb_entry_url(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s/%s.%d",g_baseurl,blog->date,blog->curnum + 1);
}

/**********************************************************************/

static void cb_entry_id(FILE *fpout,void *data)
{
  BlogDay  blog = data;
  char     output[BUFSIZ];
  char    *p;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  sprintf(output,"%s.%d",blog->date,blog->curnum + 1);
  for (p = output ; *p ; p++)
    if (*p == '/') *p = '.';
  fprintf(fpout,"%s",output);
}

/**********************************************************************/

static void cb_entry_date(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",blog->date);
}

/*********************************************************************/

static void cb_entry_number(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%d",blog->curnum + 1);
}

/**********************************************************************/

static void cb_entry_title(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",blog->entries[blog->curnum]->title);
}

/***********************************************************************/

static void cb_entry_class(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",blog->entries[blog->curnum]->class);
}

/***********************************************************************/

static void cb_entry_author(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s",blog->entries[blog->curnum]->author);
}

/*************************************************************************/

static void token_print_tag(Buffer output,HtmlToken token)
{
  struct pair *p;
  
  ddt(output != NULL);
  ddt(token  != NULL);
  
  BufferFormatWrite(output,"$","<%a",HtmlParseValue(token));
  for(
       p = HtmlParseFirstOption(token);
       NodeValid(&p->node);
       p = (struct pair *)NodeNext(&p->node)
     )
  {
    BufferFormatWrite(output,"$ $"," %a=\"%b\"",p->name,p->value);
  }
  BufferFormatWrite(output,"",">"); 
}

/***************************************************************/

static void token_print_string(Buffer output,HtmlToken token)
{
  size_t s;
  
  s = strlen(HtmlParseValue(token));
  if (s)
    BufferWrite(output,HtmlParseValue(token),&s);
}

/**************************************************************/

static void token_print_comment(Buffer output,HtmlToken token)
{
  BufferFormatWrite(output,"$","<!%a>",HtmlParseValue(token));
}

/**************************************************************/

static void fixup_uri(BlogDay blog,HtmlToken token,const char *attrib)
{
  struct pair *src;
  struct pair *np;
  
  ddt(token  != NULL);
  ddt(attrib != NULL);
  
  src = HtmlParseGetPair(token,attrib);
  if (
       (src != NULL)
       && (src->value[0] != '/')
       && (strncmp(src->value,"http:",5)        != 0)
       && (strncmp(src->value,"https:",6)       != 0)
       && (strncmp(src->value,"mailto:",7)      != 0)
       && (strncmp(src->value,"ftp:",4)         != 0)
       && (strncmp(src->value,"javascript:",11) != 0)
       && (strncmp(src->value,"nntp:",5)        != 0)
       && (strncmp(src->value,"news:",5)        != 0)
       && (strncmp(src->value,"file:",5)	!= 0)
     )
  {
    char        buffer[BUFSIZ];
    const char *baseurl;
    
    /*-----------------------------------------------------
    ; Which URL to use?  Full or partial?
    ;-----------------------------------------------------*/
    
    if (m_fullurl)
      baseurl = g_fullbaseurl;
    else
      baseurl = g_baseurl;
      
    /*---------------------------------------------------------
    ; all this to reassign the value, without ``knowing'' how
    ; the pair stuff actually works.  Perhaps add a PairReValue()
    ; call or something?
    ;-------------------------------------------------------------*/

    if (src->value[0] == '/')
      sprintf(buffer,"%s%s",baseurl,src->value);
    else if (isdigit(src->value[0]))
      sprintf(buffer,"%s/%s",baseurl,src->value);
    else
#ifdef PARALLEL_HACK
    {
      /*-----------------------------------------------------------
      ; this hack will work for now since it's not possible	XXX
      ; (through the configuration file) to select absolute
      ; URLs, but when that changes, so will this.  The non-hack
      ; version will work though.
      ;------------------------------------------------------------*/
      
      if (strstr(src->value,"parallel") != NULL)
      {
        if (strstr(g_scriptname,"parallel") != NULL)
	{
	  char *ps = strchr(src->value,'/');
	  if (ps)
	    sprintf(buffer,"%s",ps);
	  else
	    sprintf(buffer,"/");
	}
	else
	  sprintf(buffer,"%s/%s",baseurl,src->value);
      }
      else
        sprintf(buffer,"%s/%s/%s",baseurl,blog->date,src->value);
    }
#else
      sprintf(buffer,"%s/%s/%s",baseurl,blog->date,src->value);
#endif

    np = PairCreate(attrib,buffer);
    NodeInsert(&src->node,&np->node);
    NodeRemove(&src->node);
    PairFree(src);
  }
}

/***************************************************************/

static void cb_entry_body(FILE *fpout,void *data)
{
  Buffer    membuf;
  Buffer    linebuf;
  Buffer    output;
  HtmlToken token;
  BlogDay   blog = data;
  BlogEntry entry;
  int       fh;
  int       rc;
  int       t;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);

  fflush(fpout);
  fh    = fileno(fpout);
  entry = blog->entries[blog->curnum];
  
  rc = FHBuffer(&output,fh);
  if (rc != ERR_OKAY) return;
  
  rc = MemoryBuffer(&membuf,entry->body,entry->bsize);
  if (rc != ERR_OKAY) return;
  
  rc = LineBuffer(&linebuf,membuf);
  if (rc != ERR_OKAY) return;
  
  rc = HtmlParseNew(&token,linebuf);
  if (rc != ERR_OKAY) return;
  
  while((t = HtmlParseNext(token)) != T_EOF)
  {
    if (t == T_TAG)
    {
      /*---------------------------------------------
      ; checked in order of *my* usage.
      ;----------------------------------------------*/
      
      if (strcmp(HtmlParseValue(token),"A") == 0)
        fixup_uri(blog,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"BLOCKQUOTE") == 0)
        fixup_uri(blog,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"IMG") == 0)
      {
        fixup_uri(blog,token,"SRC");
        fixup_uri(blog,token,"LONGDESC");
        fixup_uri(blog,token,"USEMAP");
      }
      else if (strcmp(HtmlParseValue(token),"Q") == 0)
        fixup_uri(blog,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"INS") == 0)
        fixup_uri(blog,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"DEL") == 0)
        fixup_uri(blog,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"FORM") == 0)
        fixup_uri(blog,token,"ACTION");
      else if (strcmp(HtmlParseValue(token),"INPUT") == 0)
      {
        fixup_uri(blog,token,"SRC");
        fixup_uri(blog,token,"USEMAP");
      }
      else if (strcmp(HtmlParseValue(token),"AREA") == 0)
        fixup_uri(blog,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"OBJECT") == 0)
      {
        fixup_uri(blog,token,"CLASSID");
        fixup_uri(blog,token,"CODEBASE");
        fixup_uri(blog,token,"DATA");
        fixup_uri(blog,token,"ARCHIVE");
        fixup_uri(blog,token,"USEMAP");
      }
      
      /*-----------------------------------------------------
      ; elements that can only appear in the <HEAD> section
      ; so commented out for now but here for reference
      ;------------------------------------------------------*/
      
#if 0
      else if (strcmp(HtmlParseValue(token),"LINK") == 0)
        fixup_uri(blog,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"BASE") == 0)
        fixup_uri(blog,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"HEAD") == 0)
        fixup_uri(blog,token,"PROFILE");
      else if (strcmp(HtmlParseValue(token),"SCRIPT") == 0)
      {
        fixup_uri(blog,token,"SRC");
        fixup_uri(blog,token,"FOR");
      }
#endif

      token_print_tag(output,token);
    }
    else if (t == T_STRING)
      token_print_string(output,token);
    else if (t == T_COMMENT)
      token_print_comment(output,token);
  }
  
  HtmlParseFree(&token);
  BufferFree(&linebuf);
  BufferFree(&membuf);
  BufferFree(&output);
}

/**********************************************************************/

static void cb_cond_hr(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navigation && (m_navunit == PART))
    return;

  if (m_reverse)
  {
    if (blog->curnum != blog->number - 1)
      fprintf(fpout,"<hr class=\"next\">");
  }
  else
  {
    if (blog->curnum)
      fprintf(fpout,"<hr class=\"next\">");
  }
}

/*********************************************************************/

static void cb_rss_pubdate(FILE *fpout,void *data)
{
  time_t     now;
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  now = time(NULL);
  ptm = gmtime(&now);
  strftime(buffer,BUFSIZ,"%a, %d %b %Y %H:%M:%S GMT",ptm);
  fputs(buffer,fpout);
}

/********************************************************************/

static void cb_rss_url(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s/",g_fullbaseurl);
}

/*******************************************************************/

static void cb_item(FILE *fpout,void *data)
{
  List    *plist = data;
  BlogDay  day;
  int      i;
  int      items;
  
  items = 0;
  
  for (
        day = (BlogDay)ListGetHead(plist) ;
        NodeValid(&day->node);
        day = (BlogDay)NodeNext(&day->node)
      )
  {
    if (m_reverse)
    {
      for (i = day->endentry ; (items < g_rssitems) && (i >= day->stentry) ; i--)
      {
        day->curnum = i;
        BlogEntryRead(day->entries[i]);
        generic_cb("item",fpout,day);
        items++;
      }
    }
    else
    {
      for (i = day->stentry ; (items < g_rssitems) && (i <= day->endentry) ; i++)
      {
        day->curnum = i;
	BlogEntryRead(day->entries[i]);
        generic_cb("item",fpout,day);
        items++;
      }
    }
  }
}

/******************************************************************/

static void cb_item_url(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s/%s%s.%d",g_fullbaseurl,g_baseurl,blog->date,blog->curnum + 1);
}

/********************************************************************/

static void cb_navigation_link(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navigation == FALSE) return;
  generic_cb("navigation.link",fpout,data);
}

/********************************************************************/

static void cb_navigation_link_next(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navnext == FALSE) return;
  generic_cb("navigation.link.next",fpout,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navprev == FALSE) return;
  generic_cb("navigation.link.prev",fpout,data);
}

/*******************************************************************/

static void cb_navigation_bar(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navigation == FALSE) return;
  generic_cb("navigation.bar",fpout,data);
}

/*******************************************************************/

static void cb_navigation_bar_next(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navnext == FALSE) return;
  generic_cb("navigation.bar.next",fpout,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navprev == FALSE) return;
  generic_cb("navigation.bar.prev",fpout,data);
}

/*******************************************************************/

static void cb_navigation_current(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  if (m_navunit != INDEX) return;
  generic_cb("navigation.current",fpout,data);
}

/********************************************************************/

static void cb_navigation_next_url(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  print_nav_url(fpout,&m_next,m_navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  print_nav_url(fpout,&m_previous,m_navunit);
}

/********************************************************************/

static void cb_navigation_current_url(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  print_nav_url(fpout,&m_now,MONTH);
}

/*********************************************************************/

static void print_nav_url(FILE *fpout,struct tm *ptm,int unit)
{
  struct tm date;
  
  ddt(fpout != NULL);
  ddt(ptm   != NULL);
  
  date = *ptm;
  tm_to_blog(&date);
  
  fprintf(fpout,"%s/",g_baseurl);
  switch(unit)
  {
    case YEAR: 
         fprintf(fpout,"%4d",date.tm_year);
         break;
    case MONTH:
         fprintf(fpout,"%4d/%d",date.tm_year,date.tm_mon);
         break;
    case DAY:  
         fprintf(fpout,"%4d/%d/%d",date.tm_year,date.tm_mon,date.tm_mday);
         break;
    case PART:
         fprintf(fpout,"%4d/%d/%d.%d",date.tm_year,date.tm_mon,date.tm_mday,date.tm_hour);
         break;
    default:
         ddt(0);
  }
}

/*******************************************************************/

static void cb_begin_year(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%4d",m_begin.tm_year + 1900);
}

/*******************************************************************/

static void cb_now_year(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%4d",m_now.tm_year + 1900);
}

/*******************************************************************/

static void cb_update_time(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(
           fpout,
           "%4d-%02d-%02dT%02d:%02d%+03d:%02d",
           m_updatetime.tm_year + 1900,
           m_updatetime.tm_mon  + 1,
           m_updatetime.tm_mday,
           m_updatetime.tm_hour,
           m_updatetime.tm_min,
           g_tzhour,
           g_tzmin
         );
}

/*******************************************************************/

static void cb_update_type(FILE *fpout,void *data)
{
  fprintf(fpout,"%s",m_updatetype);
}

/*******************************************************************/

static void cb_robots_index(FILE *fpout,void *data)
{
  if (m_navunit == PART)
    fprintf(fpout,"index");
  else
    fprintf(fpout,"noindex");
}

/********************************************************************/

static void cb_comments(FILE *fpout,void *data)
{
  if (m_navunit != PART)
    return;
  
  generic_cb("comments",fpout,data);
}

/*******************************************************************/

static void cb_comments_body(FILE *fpout,void *data)
{
  BlogDay  blog = data;
  char     fname[BUFSIZ];
  FILE    *fp;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  sprintf(fname,"%s/%d.comments",blog->date,blog->curnum + 1);
  fp = fopen(fname,"r");
  if (fp == NULL) return;
  
  while(1)
  {
    char   buffer[BUFSIZ];
    size_t s;
    
    s = fread(buffer,sizeof(char),BUFSIZ,fp);
    if (s == 0) break;
    fwrite(buffer,sizeof(char),s,fpout);
  }
  
  fclose(fp);
}

/*******************************************************************/

static void cb_comments_filename(FILE *fpout,void *data)
{
  BlogDay blog = data;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  fprintf(fpout,"%s/%d.comments",blog->date,blog->curnum + 1);
}

/*******************************************************************/

static void cb_comments_check(FILE *fpout,void *data)
{
  BlogDay     blog = data;
  struct stat status;
  char        fname[BUFSIZ];
  int         rc;
  
  ddt(fpout != NULL);
  ddt(data  != NULL);
  
  sprintf(fname,"%s/%d.comments",blog->date,blog->curnum + 1);
  rc = stat(fname,&status);
  if (rc == -1)
  {
    fprintf(fpout,"No ");
  }
  fprintf(fpout,"Comments");
}

/********************************************************************/

#if 0
static void cb_calendar(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
}

/*******************************************************************/

static void cb_calendar_header(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
}

/*******************************************************************/

static void cb_calendar_days(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
}

/*******************************************************************/

static void cb_calendar_item(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
}

/*******************************************************************/

static void cb_calendar_row(FILE *fpout,void *data)
{
  ddt(fpout != NULL);
  ddt(data  != NULL);
}

#endif

