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

#include <sys/stat.h>
#include <unistd.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/htmltok.h>
#include <cgilib/stream.h>
#include <cgilib/util.h>
#include <cgilib/cgi.h>
#include <cgilib/chunk.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "wbtum.h"
#include "globals.h"
#include "frontend.h"
#include "fix.h"
#include "blogutil.h"

#define max(a,b)	((a) > (b)) ? (a) : (b)

/*****************************************************************/

static void	cb_blog_script			(Stream,void *);
static void	cb_blog_url			(Stream,void *);
#if 0
static void	cb_blog_locallink		(Stream,void *);
static void	cb_blog_body			(Stream,void *);
static void	cb_blog_entry_locallinks	(Stream,void *);
static void	cb_blog_date			(Stream,void *);
static void	cb_blog_fancy			(Stream,void *);
static void	cb_blog_normal			(Stream,void *);
#endif
static void	cb_blog_name			(Stream,void *);
static void	cb_blog_adtag			(Stream,void *);
static void	cb_blog_adtag_entity		(Stream,void *);

static void	cb_date_day			(Stream,void *);
static void	cb_date_day_url			(Stream,void *);
static void	cb_date_day_normal		(Stream,void *);

static void	cb_entry			(Stream,void *);
static void	cb_entry_id			(Stream,void *);
static void	cb_entry_url			(Stream,void *);
static void	cb_entry_cond_date		(Stream,void *);
static void	cb_entry_date			(Stream,void *);
static void	cb_entry_pubdate		(Stream,void *);
#if 0
static void	cb_entry_number			(Stream,void *);
#endif
static void	cb_entry_title			(Stream,void *);
static void	cb_entry_class			(Stream,void *);
static void	cb_entry_adtag			(Stream,void *);
static void	cb_entry_author			(Stream,void *);
static void	cb_entry_name			(Stream,void *);
static void	cb_entry_body			(Stream,void *);
static void	cb_entry_body_entified		(Stream,void *);

static void	cb_cond_hr			(Stream,void *);
static void	cb_cond_blog_title		(Stream,void *);

static void	cb_rss_pubdate			(Stream,void *);
static void	cb_rss_url			(Stream,void *);
static void	cb_rss_item			(Stream,void *);
static void	cb_rss_item_url			(Stream,void *);

static void	cb_atom_entry			(Stream,void *);
static void	cb_atom_categories		(Stream,void *);
static void	cb_atom_category		(Stream,void *);

static void	cb_navigation_link		(Stream,void *);
static void     cb_navigation_link_next		(Stream,void *);
static void     cb_navigation_link_prev		(Stream,void *);
static void	cb_navigation_current		(Stream,void *);
static void	cb_navigation_current_url	(Stream,void *);
static void	cb_navigation_bar		(Stream,void *);
static void	cb_navigation_bar_next		(Stream,void *);
static void	cb_navigation_bar_prev		(Stream,void *);
static void	cb_navigation_next_url		(Stream,void *);
static void	cb_navigation_prev_url		(Stream,void *);

static void	cb_comments			(Stream,void *);
static void	cb_comments_body		(Stream,void *);
static void	cb_comments_filename		(Stream,void *);
static void	cb_comments_check		(Stream,void *);

static void	cb_begin_year			(Stream,void *);
static void	cb_now_year			(Stream,void *);
static void     cb_update_time			(Stream,void *);
static void     cb_update_type			(Stream,void *);
static void	cb_robots_index			(Stream,void *);

static void	cb_edit				(Stream,void *);
static void	cb_edit_author			(Stream,void *);
static void	cb_edit_title			(Stream,void *);
static void	cb_edit_date			(Stream,void *);
static void	cb_edit_class			(Stream,void *);
static void	cb_edit_email			(Stream,void *);
static void	cb_edit_filter			(Stream,void *);
static void	cb_edit_body			(Stream,void *);

static void	cb_xyzzy			(Stream,void *);

static void	print_nav_url		(Stream,struct btm *,int);
static void	print_nav_name		(Stream,struct btm *,int,char);
static void	fixup_uri		(BlogEntry,HtmlToken,const char *);

/************************************************************************/

struct chunk_callback  m_callbacks[] =
{
  { "blog.script"		, cb_blog_script		} ,
  { "blog.url"			, cb_blog_url			} ,
#if 0
  { "blog.locallink"		, cb_blog_locallink		} ,
  { "blog.body"			, cb_blog_body			} ,
  { "blog.entry.locallinks"	, cb_blog_entry_locallinks	} ,
  { "blog.date"			, cb_blog_date			} ,
#endif
  { "blog.name"			, cb_blog_name			} ,
#if 0
  { "blog.date.fancy"		, cb_blog_fancy			} ,
  { "blog.date.normal"		, cb_blog_normal		} ,
#endif
  { "blog.adtag"		, cb_blog_adtag			} ,
  { "blog.adtag.entity"		, cb_blog_adtag_entity		} ,

  { "date.day"			, cb_date_day			} ,
  { "date.day.url"		, cb_date_day_url		} ,
  { "date.day.normal"		, cb_date_day_normal		} ,

  { "entry"			, cb_entry			} ,
  { "entry.id"			, cb_entry_id			} ,
  { "entry.url"			, cb_entry_url			} ,
  { "entry.cond.date"		, cb_entry_cond_date		} ,
  { "entry.date"		, cb_entry_date			} ,
  { "entry.pubdate"		, cb_entry_pubdate		} ,
  { "entry.name"		, cb_entry_name			} ,
#if 0
  { "entry.number"		, cb_entry_number		} ,
#endif
  { "entry.title"		, cb_entry_title		} ,
  { "entry.class"		, cb_entry_class		} ,
  { "entry.adtag"		, cb_entry_adtag		} ,
  { "entry.author"		, cb_entry_author		} ,
  { "entry.body"		, cb_entry_body			} ,
  { "entry.body.entified"	, cb_entry_body_entified	} ,
  
  { "comments"			, cb_comments			} ,
  { "comments.body"		, cb_comments_body		} ,
  { "comments.filename"		, cb_comments_filename		} ,
  { "comments.check"		, cb_comments_check		} ,
  
  { "cond.hr"			, cb_cond_hr			} ,
  { "cond.blog.title"		, cb_cond_blog_title		} ,

  { "rss.pubdate"		, cb_rss_pubdate		} ,
  { "rss.url"			, cb_rss_url			} ,
  { "rss.item"			, cb_rss_item			} ,
  { "rss.item.url"		, cb_rss_item_url		} ,

  { "atom.entry"		, cb_atom_entry			} ,
  { "atom.categories"		, cb_atom_categories		} ,
  { "atom.category"		, cb_atom_category		} ,

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

  { "begin.year"		, cb_begin_year			} ,
  { "now.year"			, cb_now_year			} ,
  
  { "update.time"               , cb_update_time                } ,
  { "update.type"               , cb_update_type                } ,
  
  { "robots.index"		, cb_robots_index		} ,

  { "edit"			, cb_edit			} ,
  { "edit.author"		, cb_edit_author		} ,
  { "edit.title"		, cb_edit_title			} ,
  { "edit.date"			, cb_edit_date			} ,
  { "edit.class"		, cb_edit_class			} ,
  { "edit.email"		, cb_edit_email			} ,
  { "edit.filter"		, cb_edit_filter		} ,
  { "edit.body"			, cb_edit_body			} ,

  { "xyzzy"			, cb_xyzzy			} 

};
size_t m_cbnum = sizeof(m_callbacks) / sizeof(struct chunk_callback);

/*************************************************************************/

void generic_cb(const char *which,Stream out,void *data)
{
  Chunk templates;
  
  ddt(which != NULL);
  ddt(out   != NULL);
  
  ChunkNew(&templates,g_templates,m_callbacks,m_cbnum);
  ChunkProcess(templates,which,out,data);
  ChunkFree(templates);
  StreamFlush(out);
}

/*********************************************************************/

static void cb_blog_script(Stream out,void *data)
{
  char *script;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  script = spc_getenv("SCRIPT_NAME");
  LineS(out,script);
  MemFree(script);
}

/**********************************************************************/

static void cb_blog_url(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,DAY);
}

/*************************************************************************/

#if 0
static void cb_blog_locallink(Stream out,void *data)
{
  List    *plist = data;
  BlogDay  day;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  for (
        day = (BlogDay)ListGetHead(plist);
        NodeValid(&day->node);
        day = (BlogDay)NodeNext(&day->node)
      )
  {
    generic_cb("blog.locallink",out,day);
  }
}
#endif

/**********************************************************************/

#if 0
static void cb_blog_body(Stream out,void *data)
{
  List    *plist = data;
  BlogDay  day;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  if (gd.htmldump)
  {
    StreamCopy(out,gd.htmldump);
    return;
  }
    
  for (
        day = (BlogDay)ListGetHead(plist);
        NodeValid(&day->node);
        day = (BlogDay)NodeNext(&day->node)
      )
  {
    generic_cb("blog.body",out,day);
  }
}
#endif

/*********************************************************************/

#if 0
static void cb_blog_entry_locallinks(Stream out,void *data)
{
  BlogDay blog = data;
  int            i;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.reverse)
  {
    for (i = blog->endentry ; i >= blog->stentry ; i--)
    {
      blog->curnum = i;
      generic_cb("blog.entry.locallinks",out,data);
    }
  }
  else
  {
    for (i = blog->stentry ; i <= blog->endentry ; i++)
    {
      blog->curnum = i; 
      generic_cb("blog.entry.locallinks",out,data);
    }
  }
}
#endif

/********************************************************************/
#if 0
static void cb_blog_date(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  
  
  LineS(out,blog->date);
}
#endif

/********************************************************************/

static void cb_blog_name(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineS(out,c_name);
}

/*********************************************************************/

#if 0
static void cb_blog_fancy(Stream out,void *data)
{
  BlogDay    blog = data;
  struct tm  day;
  char      *p;
  char       dayname[BUFSIZ];
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  tm_init(&day);
  
  day.tm_year = strtoul(blog->date,&p,10) - 1900 ; p++;
  day.tm_mon  = strtoul(p,&p,10) - 1;              p++;
  day.tm_mday = strtoul(p,&p,10);
  
  mktime(&day);
  strftime(dayname,BUFSIZ,"%A",&day); /* all that just to get the day name */
  
  LineSFormat(out,"$ i $","%a the %b<sup>%c</sup>",dayname,day.tm_mday,nth(day.tm_mday));
}
#endif

/************************************************************************/
#if 0
static void cb_blog_normal(Stream out,void *data)
{
  BlogDay    blog = data;
  struct tm  day;
  char       buffer[BUFSIZ];
  char      *p;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  tm_init(&day);

  day.tm_year = strtoul(blog->date,&p,10) - 1900 ; p++;
  day.tm_mon  = strtoul(p,&p,10) - 1;              p++;
  day.tm_mday = strtoul(p,&p,10);

  mktime(&day);
  
  strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&day);
  LineS(out,buffer);
}
#endif

/***********************************************************************/

static void cb_blog_adtag(Stream out,void *data)
{
  char *tag;

  ddt(out  != NULL);

  tag = UrlEncodeString(gd.adtag);
  LineS(out,tag);
  MemFree(tag);
}

/*********************************************************************/

static void cb_blog_adtag_entity(Stream out,void *data)
{
  Stream entityout;

  ddt(out  != NULL);
  ddt(data != NULL);
  
  entityout = EntityStreamWrite(out);
  if (entityout == NULL) return;
  LineS(entityout,gd.adtag);
  StreamFree(entityout);
}

/********************************************************************/

static void cb_entry(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);

  if (data == NULL)
  {
    if (gd.htmldump)
      StreamCopy(out,gd.htmldump);
    return;
  }

  for
  (
    entry = (BlogEntry)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(&cbd->list)
  )
  {
    cbd->entry = entry;
    generic_cb("entry",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/**********************************************************************/

static void cb_entry_url(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  entry = cbd->entry;
  print_nav_url(out,&entry->when,PART);
}

/**********************************************************************/

static void cb_entry_id(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  entry = cbd->entry;
  print_nav_name(out,&entry->when,DAY,'-');
}

/**********************************************************************/

static void cb_entry_date(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  print_nav_url(out,&cbd->entry->when,DAY);  
}

/*********************************************************************/

static void cb_entry_pubdate(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry  entry;
  time_t     ts;
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  entry = cbd->entry;
  ts    = entry->timestamp;
  ptm   = localtime(&ts);
  
  sprintf(
  	buffer,
  	"%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
  	ptm->tm_year + 1900,
  	ptm->tm_mon + 1,
  	ptm->tm_mday,
  	ptm->tm_hour,
  	ptm->tm_min,
  	ptm->tm_sec,
  	c_tzhour,
  	c_tzmin
  );

  LineS(out,buffer);
}

/********************************************************************/

#if 0
static void cb_entry_number(Stream out,void *data)
{
  BlogDay blog = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineSFormat(out,"i","%a",blog->curnum + 1);
}
#endif

/**********************************************************************/

static void cb_entry_title(Stream out,void *data)
{
  struct callback_data *cbd = data;
    
  ddt(out  != NULL);
  ddt(data != NULL);

  LineS(out,cbd->entry->title);  
}

/***********************************************************************/

static void cb_entry_class(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  LineS(out,cbd->entry->class);  
}

/***********************************************************************/

static void cb_entry_adtag(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineS(out,"Soon young Skywalker.  Soon.");
}

/**********************************************************************/

static void cb_entry_author(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  LineS(out,cbd->entry->author);  
}

/*************************************************************************/

static void fixup_uri(BlogEntry entry,HtmlToken token,const char *attrib)
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
    
    if (gd.f.fullurl)
      baseurl = c_fullbaseurl;
    else
      baseurl = c_baseurl;
      
    /*---------------------------------------------------------
    ; all this to reassign the value, without ``knowing'' how
    ; the pair stuff actually works.  Perhaps add a PairReValue()
    ; call or something?
    ;-------------------------------------------------------------*/

    /*---------------------------------------------------
    ; check to see if baseurl ends in a '/', and if not, 
    ; we then add one, otherwise, don't.
    ;--------------------------------------------------*/

    /*---------------------------------------------------------
    ; XXX - Okay, isolated the bug.  If baseurl ends in a '/', and
    ; src->value does not start with a '/', we end up with
    ; a double '//'.  We need to make sure we canonicalize
    ; all partial urls to either end in a '/', or not end in a
    ; '/'.
    ;----------------------------------------------------------*/
    
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
        if (strstr(c_scriptname,"parallel") != NULL)
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
	sprintf(
		buffer,
		"%s/%04d/%02d/%02d/%s",
		baseurl,
		entry->when.year,
		entry->when.month,
		entry->when.day,
		src->value
	);
    }
#else
      sprintf(
      	buffer,
	"%s/%04d/%02d/%02d/%s",
	baseurl,
	entry->when.year,
	entry->when.month,
	entry->when.day,
	src->value
      );
#endif

    np = PairCreate(attrib,buffer);
    NodeInsert(&src->node,&np->node);
    NodeRemove(&src->node);
    PairFree(src);
  }
}

/***************************************************************/

static void cb_entry_body(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  Stream                in;
  HtmlToken             token;
  int                   rc;
  int                   t;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  entry = cbd->entry;
  in    = MemoryStreamRead(entry->body,strlen(entry->body));
  rc    = HtmlParseNew(&token,in);
  
  if (rc != ERR_OKAY)
  {
    StreamFree(in);
    return;
  }
  
  while((t = HtmlParseNext(token)) != T_EOF)
  {
    if (t == T_TAG)
    {
      /*---------------------------------------------
      ; checked in order of *my* usage.
      ;----------------------------------------------*/
      
      if (strcmp(HtmlParseValue(token),"A") == 0)
        fixup_uri(entry,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"BLOCKQUOTE") == 0)
        fixup_uri(entry,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"IMG") == 0)
      {
        fixup_uri(entry,token,"SRC");
        fixup_uri(entry,token,"LONGDESC");
        fixup_uri(entry,token,"USEMAP");
      }
      else if (strcmp(HtmlParseValue(token),"Q") == 0)
        fixup_uri(entry,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"INS") == 0)
        fixup_uri(entry,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"DEL") == 0)
        fixup_uri(entry,token,"CITE");
      else if (strcmp(HtmlParseValue(token),"FORM") == 0)
        fixup_uri(entry,token,"ACTION");
      else if (strcmp(HtmlParseValue(token),"INPUT") == 0)
      {
        fixup_uri(entry,token,"SRC");
        fixup_uri(entry,token,"USEMAP");
      }
      else if (strcmp(HtmlParseValue(token),"AREA") == 0)
        fixup_uri(entry,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"OBJECT") == 0)
      {
        fixup_uri(entry,token,"CLASSID");
        fixup_uri(entry,token,"CODEBASE");
        fixup_uri(entry,token,"DATA");
        fixup_uri(entry,token,"ARCHIVE");
        fixup_uri(entry,token,"USEMAP");
      }
      
      /*-----------------------------------------------------
      ; elements that can only appear in the <HEAD> section
      ; so commented out for now but here for reference
      ;------------------------------------------------------*/
      
#if 0
      else if (strcmp(HtmlParseValue(token),"LINK") == 0)
        fixup_uri(entry,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"BASE") == 0)
        fixup_uri(entry,token,"HREF");
      else if (strcmp(HtmlParseValue(token),"HEAD") == 0)
        fixup_uri(entry,token,"PROFILE");
      else if (strcmp(HtmlParseValue(token),"SCRIPT") == 0)
      {
        fixup_uri(entry,token,"SRC");
        fixup_uri(entry,token,"FOR");
      }
#endif
      HtmlParsePrintTag(token,out);
    }
    else if (t == T_STRING)
      LineS(out,HtmlParseValue(token));
    else if (t == T_COMMENT)
      LineSFormat(out,"$","<!%a>",HtmlParseValue(token));
  }
  
  HtmlParseFree(&token);
  StreamFree(in);  
}

/**********************************************************************/

static void cb_entry_body_entified(Stream out,void *data)
{
  Stream eout;
  
  eout = EntityStreamWrite(out);
  if (eout == NULL) return;
  cb_entry_body(eout,data);
  StreamFree(eout);  
}

/*********************************************************************/

static void cb_cond_hr(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.navigation && (gd.navunit == PART))
    return;

  entry = cbd->entry;
  if (btm_cmp_date(&entry->when,&cbd->last) == 0)
  {
    if (entry->when.part != cbd->last.part)
      LineS(out,"<hr class=\"next\">");
  }
}

/*********************************************************************/

static void cb_cond_blog_title(Stream out,void *data)
{
  List *plist = data;
  BlogEntry     entry;
  
  ddt(out  != NULL);
  
  if (gd.navunit == PART)
  {
    entry = (BlogEntry)ListGetHead(plist);
    LineSFormat(
    	out,
    	"$",
    	"%a - ",
    	entry->title
    );
  }
}

/*********************************************************************/

static void cb_rss_pubdate(Stream out,void *data)
{
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  ptm = gmtime(&gd.tst);
  strftime(buffer,BUFSIZ,"%a, %d %b %Y %H:%M:%S GMT",ptm);
  LineS(out,buffer);
}

/********************************************************************/

static void cb_rss_url(Stream out,void *data)
{
  ddt(out  != NULL);
  
  LineSFormat(out,"$","%a/",c_fullbaseurl);
}

/*******************************************************************/

static void cb_rss_item(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  for
  (
    entry = (BlogEntry)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(&cbd->list)
  )
  {
    cbd->entry = entry;
    generic_cb("item",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/******************************************************************/

static void cb_rss_item_url(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineSFormat(out,"$","%a/",c_fullbaseurl);
  print_nav_url(out,&cbd->entry->when,PART);
  
#if 0  
  LineSFormat(out,"$ $ $ i","%a/%b%c.%d",c_fullbaseurl,c_baseurl,blog->date,blog->curnum + 1);
#endif
}

/********************************************************************/

static void cb_atom_entry(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  for
  (
    entry = (BlogEntry)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(&cbd->list)
  )
  {
    cbd->entry = entry;
    generic_cb("entry",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/************************************************************************/

static void cb_atom_categories(Stream out,void *data)
{
  struct callback_data *cbd = data;
  String  *cats;
  char    *tag;
  size_t   i;
  size_t   num;

  ddt(out  != NULL);
  ddt(data != NULL);
  
  cats = tag_split(&num,cbd->entry->class);

  for (i = 0 ; i < num ; i++)
  {
    tag = fromstring(cats[i]);
    generic_cb("categories",out,tag);
    MemFree(tag);
  }

  MemFree(cats);
}

/************************************************************************/

static void cb_atom_category(Stream out,void *data)
{
  Stream eout;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  eout = EntityStreamWrite(out);
  if (eout == NULL) return;
  LineS(eout,data);
  StreamFree(eout);
}

/****************************************************************/

static void cb_navigation_link(Stream out,void *data)
{
  ddt(out  != NULL);
  
  if (gd.f.navigation == FALSE) return;
  generic_cb("navigation.link",out,data);
}

/********************************************************************/

static void cb_navigation_link_next(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.navnext == FALSE) return;
  generic_cb("navigation.link.next",out,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.navprev == FALSE) return;
  generic_cb("navigation.link.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_bar(Stream out,void *data)
{
  ddt(out  != NULL);
  
  if (gd.f.navigation == FALSE) return;
  generic_cb("navigation.bar",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_next(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.navnext == FALSE) return;
  generic_cb("navigation.bar.next",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.f.navprev == FALSE) return;
  generic_cb("navigation.bar.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_current(Stream out,void *data)
{
  ddt(out  != NULL);
  
  if (gd.navunit != INDEX) return;
  generic_cb("navigation.current",out,data);
}

/********************************************************************/

static void cb_navigation_next_url(Stream out,void *data)
{
  struct btm tmp;

  ddt(out  != NULL);
  ddt(data != NULL);

  tmp = gd.next;
  print_nav_url(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(Stream out,void *data)
{
  struct btm tmp;

  ddt(out  != NULL);
  ddt(data != NULL);
  
  tmp = gd.previous;
  print_nav_url(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_current_url(Stream out,void *data)
{
  struct btm tmp;

  ddt(out  != NULL);
  
  tmp = gd.now;
  print_nav_url(out,&tmp,MONTH);
}

/*********************************************************************/

static void print_nav_url(Stream out,struct btm *date,int unit)
{
  ddt(out  != NULL);
  ddt(date != NULL);
  
  LineSFormat(out,"$","%a/",c_baseurl);
  print_nav_name(out,date,unit,'/');
}

/*******************************************************************/

static void print_nav_name(Stream out,struct btm *date,int unit,char sep)
{
  ddt(out  != NULL);
  ddt(date != NULL);

  switch(unit)
  {
    case YEAR: 
         LineSFormat(out,"i4","%a",date->year);
         break;
    case MONTH:
         LineSFormat(out,"c i4 i2r0","%b%a%c",sep,date->year,date->month);
         break;
    case DAY:  
         LineSFormat(out,"c i4 i2r0 i2r0","%b%a%c%a%d",sep,date->year,date->month,date->day);
         break;
    case PART:
         LineSFormat(out,"c i4 i2r0 i2r0 i","%b%a%c%a%d.%e",sep,date->year,date->month,date->day,date->part);
         break;
    default:
         ddt(0);
  }
}

/********************************************************************/
 
static void cb_begin_year(Stream out,void *data)
{
  ddt(out  != NULL);
  
  LineSFormat(out,"i4","%a",gd.begin.year);
}

/*******************************************************************/

static void cb_now_year(Stream out,void *data)
{
  ddt(out  != NULL);
 
  LineSFormat(out,"i4","%a",gd.now.year); 
}

/*******************************************************************/

static void cb_update_time(Stream out,void *data)
{
  char buffer[BUFSIZ];
  
  ddt(out  != NULL);
  
#ifndef FIXED_BROKEN_RAWFMT 

  sprintf(
  	buffer,
  	"%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
  	gd.stmst.tm_year + 1900,
  	gd.stmst.tm_mon  + 1,
  	gd.stmst.tm_mday,
	gd.stmst.tm_hour,
	gd.stmst.tm_min,
	gd.stmst.tm_sec,
	c_tzhour,
	c_tzmin
  );

  LineS(out,buffer);

#else
  
  /*--------------------------------------------------
  ; until I can fix RawFmt(), the original format
  ; string used was:
  ;
  ;	"i4 i2.2l0 i2.2l0 i2.2l0 i2.2l0 i i2.2l0",
  ;--------------------------------------------------*/
	
  LineSFormat(
  	out,
  	"i4 i2.2l0 i2.2l0 i2.2l0 i2.2l0 i i2.2l0",
  	"%a-%b-%cT%d:%e%f:%g",
        gd.updatetime.tm_year + 1900,
        gd.updatetime.tm_mon  + 1,
        gd.updatetime.tm_mday,
        gd.updatetime.tm_hour,
        gd.updatetime.tm_min,
        c_tzhour,
        c_tzmin
  );
#endif

}

/*******************************************************************/

static void cb_update_type(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineS(out,c_updatetype);
}

/*******************************************************************/

static void cb_robots_index(Stream out,void *data)
{
  ddt(out  != NULL);
  
  if ((gd.navunit == PART) || (gd.navunit == INDEX))
    LineS(out,"index");
  else
    LineS(out,"noindex");
}

/********************************************************************/

static void cb_comments(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.navunit != PART)
    return;
  
  generic_cb("comments",out,data);
}

/*******************************************************************/

static void cb_comments_body(Stream out,void *data)
{
  struct callback_data *cbd = data;
  Stream   in;
  char     fname[FILENAME_LEN];
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  sprintf(
  	fname,
  	"%04d/%02d/%02d/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
  
  in = FileStreamRead(fname);
  if (in == NULL) return;
  StreamCopy(out,in);
  StreamFree(in);
}

/*******************************************************************/

static void cb_comments_filename(Stream out,void *data)
{
  struct callback_data *cbd = data;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  LineSFormat(
  	out,
  	"i4 i2r0 i2r0 i",
  	"%a/%b/%c/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
}

/*******************************************************************/

static void cb_comments_check(Stream out,void *data)
{
  struct callback_data *cbd = data;
  struct stat status;
  char        fname[BUFSIZ];
  int         rc;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  sprintf(
  	fname,
  	"%04d/%02d/%02d/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
    
  rc = stat(fname,&status);
  if (rc == -1)
    LineS(out,"No ");

  LineS(out,"Comments");
}

/********************************************************************/

static void cb_edit(Stream out,void *data)
{
  ddt(out  != NULL);
  
  if (gd.f.edit == 0) return;
  generic_cb("edit",out,data);
}

/*********************************************************************/

static void cb_edit_author(Stream out,void *data)
{
  char *name;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  if (gd.req->origauthor != NULL)
    LineS(out,gd.req->origauthor);
  else
  {
    name = get_remote_user();
    LineS(out,name);
    MemFree(name);
  }
}

/********************************************************************/

static void cb_edit_title(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.req->title != NULL)
    LineS(out,gd.req->title);
}

/********************************************************************/

static void cb_edit_date(Stream out,void *data)
{
  time_t     now;
  struct tm *ptm;
  char       buffer[BUFSIZ];

  ddt(out  != NULL);
  ddt(data != NULL);

  if (gd.req->date != NULL)
    LineS(out,gd.req->date);
  else
  {
    now = time(NULL);
    ptm = localtime(&now);
    strftime(buffer,BUFSIZ,"%Y/%m/%d",ptm);
    LineS(out,buffer);
  }
}

/********************************************************************/

static void cb_edit_class(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.req->class)
    LineS(out,gd.req->class);
}

/********************************************************************/

static void cb_edit_email(Stream out,void *data)
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_filter(Stream out,void *data)
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_body(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  if (gd.req->origbody)
    LineS(out,gd.req->origbody);
}

/********************************************************************/

static void cb_xyzzy(Stream out,void *data)
{
  ddt(out  != NULL);
  ddt(data != NULL);
  
  LineS(out,"Nothing happens.");
}

/********************************************************************/

static void cb_date_day(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  entry = cbd->entry;
  print_nav_name(out,&entry->when,DAY,'-');
}

/*************************************************************************/

static void cb_date_day_url(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  entry = cbd->entry;
  print_nav_url(out,&entry->when,DAY);
}

/********************************************************************/

static void cb_date_day_normal(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  struct tm             day;
  char                  buffer[BUFSIZ];
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  tm_init(&day);
  entry       = cbd->entry;
  day.tm_year = entry->when.year - 1900;
  day.tm_mon  = entry->when.month - 1;
  day.tm_mday = entry->when.day;
  
  mktime(&day);
  
  strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&day);
  LineS(out,buffer);
}

/**********************************************************************/

static void cb_entry_cond_date(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);

  entry = cbd->entry;  
  
  if (btm_cmp_date(&entry->when,&cbd->last) != 0)
    generic_cb("entry.cond.date",out,data);
}

/**********************************************************************/

static void cb_entry_name(Stream out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  ddt(out  != NULL);
  ddt(data != NULL);
  
  entry = cbd->entry;
  print_nav_name(out,&entry->when,PART,'-');
}

/***********************************************************************/

