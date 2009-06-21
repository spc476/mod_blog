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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <sys/stat.h>
#include <unistd.h>

#include <cgilib6/htmltok.h>
#include <cgilib6/util.h>
#include <cgilib6/cgi.h>
#include <cgilib6/chunk.h>

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

static void	cb_ad				(FILE *,void *);
static void	cb_ad_content			(FILE *,void *);
static void	cb_atom_categories		(FILE *,void *);
static void	cb_atom_category		(FILE *,void *);
static void	cb_atom_entry			(FILE *,void *);
static void	cb_begin_year			(FILE *,void *);
static void	cb_blog_adtag			(FILE *,void *);
static void	cb_blog_adtag_entity		(FILE *,void *);
static void	cb_blog_name			(FILE *,void *);
static void	cb_blog_script			(FILE *,void *);
static void	cb_blog_url			(FILE *,void *);
static void	cb_comments			(FILE *,void *);
static void	cb_comments_body		(FILE *,void *);
static void	cb_comments_check		(FILE *,void *);
static void	cb_comments_filename		(FILE *,void *);
static void	cb_cond_blog_title		(FILE *,void *);
static void	cb_cond_hr			(FILE *,void *);
static void	cb_date_day			(FILE *,void *);
static void	cb_date_day_normal		(FILE *,void *);
static void	cb_date_day_url			(FILE *,void *);
static void	cb_edit				(FILE *,void *);
static void	cb_edit_author			(FILE *,void *);
static void	cb_edit_body			(FILE *,void *);
static void	cb_edit_class			(FILE *,void *);
static void	cb_edit_date			(FILE *,void *);
static void	cb_edit_email			(FILE *,void *);
static void	cb_edit_filter			(FILE *,void *);
static void	cb_edit_title			(FILE *,void *);
static void	cb_entry			(FILE *,void *);
static void	cb_entry_adtag			(FILE *,void *);
static void	cb_entry_author			(FILE *,void *);
static void	cb_entry_body			(FILE *,void *);
static void	cb_entry_body_entified		(FILE *,void *);
static void	cb_entry_class			(FILE *,void *);
static void	cb_entry_cond_date		(FILE *,void *);
static void	cb_entry_date			(FILE *,void *);
static void	cb_entry_id			(FILE *,void *);
static void	cb_entry_name			(FILE *,void *);
static void	cb_entry_pubdate		(FILE *,void *);
static void	cb_entry_title			(FILE *,void *);
static void	cb_entry_url			(FILE *,void *);
static void	cb_navigation_bar		(FILE *,void *);
static void	cb_navigation_bar_next		(FILE *,void *);
static void	cb_navigation_bar_prev		(FILE *,void *);
static void	cb_navigation_current		(FILE *,void *);
static void	cb_navigation_current_url	(FILE *,void *);
static void	cb_navigation_link		(FILE *,void *);
static void	cb_navigation_next_url		(FILE *,void *);
static void	cb_navigation_prev_url		(FILE *,void *);
static void	cb_now_year			(FILE *,void *);
static void	cb_overview			(FILE *,void *);
static void	cb_overview_date		(FILE *,void *);
static void	cb_overview_list		(FILE *,void *);
static void	cb_robots_index			(FILE *,void *);
static void	cb_rss_item			(FILE *,void *);
static void	cb_rss_item_url			(FILE *,void *);
static void	cb_rss_pubdate			(FILE *,void *);
static void	cb_rss_url			(FILE *,void *);
static void	cb_xyzzy			(FILE *,void *);
static void     cb_navigation_link_next		(FILE *,void *);
static void     cb_navigation_link_prev		(FILE *,void *);
static void     cb_update_time			(FILE *,void *);
static void     cb_update_type			(FILE *,void *);

static void	print_nav_url		(FILE *,struct btm *,int);
static void	print_nav_name		(FILE *,struct btm *,int,char);
static void	fixup_uri		(BlogEntry,HtmlToken,const char *);

/************************************************************************/

struct chunk_callback  m_callbacks[] =
{  
  { "ad"			, cb_ad				} ,
  { "ad.content"		, cb_ad_content			} ,
  { "atom.categories"		, cb_atom_categories		} ,
  { "atom.category"		, cb_atom_category		} ,
  { "atom.entry"		, cb_atom_entry			} ,
  { "begin.year"		, cb_begin_year			} ,
  { "blog.adtag"		, cb_blog_adtag			} ,
  { "blog.adtag.entity"		, cb_blog_adtag_entity		} ,
  { "blog.name"			, cb_blog_name			} ,
  { "blog.script"		, cb_blog_script		} ,
  { "blog.url"			, cb_blog_url			} ,
  { "comments"			, cb_comments			} ,
  { "comments.body"		, cb_comments_body		} ,
  { "comments.check"		, cb_comments_check		} ,
  { "comments.filename"		, cb_comments_filename		} ,
  { "cond.blog.title"		, cb_cond_blog_title		} ,
  { "cond.hr"			, cb_cond_hr			} ,
  { "date.day"			, cb_date_day			} ,
  { "date.day.normal"		, cb_date_day_normal		} ,
  { "date.day.url"		, cb_date_day_url		} ,
  { "edit"			, cb_edit			} ,
  { "edit.author"		, cb_edit_author		} ,
  { "edit.body"			, cb_edit_body			} ,
  { "edit.class"		, cb_edit_class			} ,
  { "edit.date"			, cb_edit_date			} ,
  { "edit.email"		, cb_edit_email			} ,
  { "edit.filter"		, cb_edit_filter		} ,
  { "edit.title"		, cb_edit_title			} ,
  { "entry"			, cb_entry			} ,
  { "entry.adtag"		, cb_entry_adtag		} ,
  { "entry.author"		, cb_entry_author		} ,
  { "entry.body"		, cb_entry_body			} ,
  { "entry.body.entified"	, cb_entry_body_entified	} ,
  { "entry.class"		, cb_entry_class		} ,
  { "entry.cond.date"		, cb_entry_cond_date		} ,
  { "entry.date"		, cb_entry_date			} ,
  { "entry.id"			, cb_entry_id			} ,
  { "entry.name"		, cb_entry_name			} ,
  { "entry.pubdate"		, cb_entry_pubdate		} ,
  { "entry.title"		, cb_entry_title		} ,
  { "entry.url"			, cb_entry_url			} ,
  { "navigation.bar"		, cb_navigation_bar		} ,
  { "navigation.bar.next"	, cb_navigation_bar_next	} ,
  { "navigation.bar.prev"	, cb_navigation_bar_prev	} ,
  { "navigation.current"	, cb_navigation_current		} ,
  { "navigation.current.url"	, cb_navigation_current_url	} ,
  { "navigation.link"		, cb_navigation_link		} ,
  { "navigation.link.next"	, cb_navigation_link_next	} ,
  { "navigation.link.prev"	, cb_navigation_link_prev	} ,
  { "navigation.next.url"	, cb_navigation_next_url	} ,
  { "navigation.prev.url"	, cb_navigation_prev_url        } ,
  { "now.year"			, cb_now_year			} ,
  { "overview"			, cb_overview			} ,
  { "overview.date"		, cb_overview_date		} ,
  { "overview.list"		, cb_overview_list		} ,
  { "robots.index"		, cb_robots_index		} ,
  { "rss.item"			, cb_rss_item			} ,
  { "rss.item.url"		, cb_rss_item_url		} ,
  { "rss.pubdate"		, cb_rss_pubdate		} ,
  { "rss.url"			, cb_rss_url			} ,
  { "update.time"               , cb_update_time                } ,
  { "update.type"               , cb_update_type                } ,
  { "xyzzy"			, cb_xyzzy			} 
};
size_t m_cbnum = sizeof(m_callbacks) / sizeof(struct chunk_callback);

/*************************************************************************/

void generic_cb(const char *which,FILE* out,void *data)
{
  Chunk templates;
  
  assert(which != NULL);
  assert(out   != NULL);
  
  templates = ChunkNew(g_templates,m_callbacks,m_cbnum);
  ChunkProcess(templates,which,out,data);
  ChunkFree(templates);
  fflush(out);
}

/*********************************************************************/

static void cb_ad(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  sprintf(
  	fname,
  	"%04d/%02d/%02d/%d.ad",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
  
  cbd->ad = fopen(fname,"r");
  if (cbd->ad == NULL) return;
  generic_cb("ad",out,data);
  fclose(cbd->ad);
  cbd->ad = NULL;
}

/**********************************************************************/

static void cb_ad_content(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /*------------------------------------------
  ; we might also do a generic_cb() here, but
  ; I would need one that takes a FILE *
  ; object ... just an idea ... 
  ;-------------------------------------------*/
  
  fcopy(out,cbd->ad);
}  
  
/*********************************************************************/  

static void cb_blog_script(FILE *out,void *data)
{
  char *script;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  script = getenv("SCRIPT_NAME");
  if (script)
    fputs(script,out);
}

/**********************************************************************/

static void cb_blog_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,DAY);
}

/*************************************************************************/

static void cb_blog_name(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(c_name,out);
}

/*********************************************************************/

static void cb_blog_adtag(FILE *out,void *data)
{
  char *tag;

  assert(out != NULL);

  tag = UrlEncodeString(gd.adtag);
  fputs(tag,out);
  free(tag);
}

/*********************************************************************/

static void cb_blog_adtag_entity(FILE *out,void *data)
{
#if 0
  Stream entityout;

  assert(out  != NULL);
  assert(data != NULL);
  
  entityout = EntityStreamWrite(out);
  if (entityout == NULL) return;
  LineS(entityout,gd.adtag);
  StreamFree(entityout);
#endif
}

/********************************************************************/

static void cb_entry(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);

  if (data == NULL)
  {
    if (gd.htmldump)
      fcopy(out,gd.htmldump);
    return;
  }

  if (gd.f.overview)
  {
    generic_cb("overview",out,data);
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

static void cb_entry_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);

  entry = cbd->entry;
  print_nav_url(out,&entry->when,PART);
}

/**********************************************************************/

static void cb_entry_id(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);

  entry = cbd->entry;
  print_nav_name(out,&entry->when,DAY,'-');
}

/**********************************************************************/

static void cb_entry_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);

  print_nav_url(out,&cbd->entry->when,DAY);  
}

/*********************************************************************/

static void cb_entry_pubdate(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  time_t                ts;
  struct tm            *ptm;
  char                  buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
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
  
  fputs(buffer,out);
}

/********************************************************************/

static void cb_entry_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
    
  assert(out  != NULL);
  assert(data != NULL);

  fputs(cbd->entry->title,out);
}

/***********************************************************************/

static void cb_entry_class(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);

  fputs(cbd->entry->class,out);  
}

/***********************************************************************/

static void cb_entry_adtag(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs("Soon young Skywalker.  Soon.",out);
}

/**********************************************************************/

static void cb_entry_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);

  fputs(cbd->entry->author,out);
}

/*************************************************************************/

static void fixup_uri(BlogEntry entry,HtmlToken token,const char *attrib)
{
  struct pair *src;
  struct pair *np;
  
  assert(token  != NULL);
  assert(attrib != NULL);
  
  src = HtmlParseGetPair(token,attrib);
  if (
       (src != NULL)
       && (src->value[0] != '/')
       && (src->value[0] != '#')
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

static void cb_entry_body(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  FILE                 *in;
  HtmlToken             token;
  int                   rc;
  int                   t;
  
  assert(out  != NULL);
  assert(data != NULL);

  entry = cbd->entry;
  in    = fmemopen(entry->body,strlen(entry->body),"r");
  token = HtmlParseNew(in);
  
  if (rc != ERR_OKAY)
  {
    fclose(in);
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
      fputs(HtmlParseValue(token),out);
    else if (t == T_COMMENT)
      fprintf(out,"<!%s>",HtmlParseValue(token));
  }
  
  HtmlParseFree(token);
  fclose(in);
}

/**********************************************************************/

static void cb_entry_body_entified(FILE *out,void *data)
{
#if 0
  Stream eout;
  
  eout = EntityStreamWrite(out);
  if (eout == NULL) return;
  cb_entry_body(eout,data);
  StreamFree(eout);  
#endif
}

/*********************************************************************/

static void cb_cond_hr(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.f.navigation && (gd.navunit == PART))
    return;

  entry = cbd->entry;
  if (btm_cmp_date(&entry->when,&cbd->last) == 0)
  {
    if (entry->when.part != cbd->last.part)
      fputs("<hr class=\"next\">",out);
  }
}

/*********************************************************************/

static void cb_cond_blog_title(FILE *out,void *data)
{
  List *plist = data;
  BlogEntry     entry;
  
  assert(out != NULL);
  
  if (gd.navunit == PART)
  {
    entry = (BlogEntry)ListGetHead(plist);
    fprintf(out,"%s - ",entry->title);
  }
}

/*********************************************************************/

static void cb_rss_pubdate(FILE *out,void *data)
{
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  ptm = gmtime(&gd.tst);
  strftime(buffer,BUFSIZ,"%a, %d %b %Y %H:%M:%S GMT",ptm);
  fputs(buffer,out);
}

/********************************************************************/

static void cb_rss_url(FILE *out,void *data)
{
  assert(out != NULL);
  
  fprintf(out,"%s/",c_fullbaseurl);
}

/*******************************************************************/

static void cb_rss_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /* XXX really?  This works?! */
  
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

static void cb_rss_item_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fprintf(out,"%s",c_fullbaseurl);
  print_nav_url(out,&cbd->entry->when,PART);
}

/********************************************************************/

static void cb_atom_entry(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /* XXX really? What's going on here? */
  
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

static void cb_atom_categories(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  String  *cats;
  char    *tag;
  size_t   i;
  size_t   num;

  assert(out  != NULL);
  assert(data != NULL);
  
  cats = tag_split(&num,cbd->entry->class);

  for (i = 0 ; i < num ; i++)
  {
    tag = fromstring(cats[i]);
    generic_cb("categories",out,tag);
    free(tag);
  }

  free(cats);
}

/************************************************************************/

static void cb_atom_category(FILE *out,void *data)
{
#if 0
  Stream eout;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  eout = EntityStreamWrite(out);
  if (eout == NULL) return;
  LineS(eout,data);
  StreamFree(eout);
#endif
}

/****************************************************************/

static void cb_navigation_link(FILE *out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navigation == false) return;
  generic_cb("navigation.link",out,data);
}

/********************************************************************/

static void cb_navigation_link_next(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.link.next",out,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.f.navprev == false) return;
  generic_cb("navigation.link.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_bar(FILE *out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navigation == false) return;
  generic_cb("navigation.bar",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_next(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.bar.next",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.f.navprev == false) return;
  generic_cb("navigation.bar.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_current(FILE *out,void *data)
{
  assert(out != NULL);
  
  if (gd.navunit != INDEX) return;
  generic_cb("navigation.current",out,data);
}

/********************************************************************/

static void cb_navigation_next_url(FILE *out,void *data)
{
  struct btm tmp;

  assert(out  != NULL);
  assert(data != NULL);

  tmp = gd.next;
  print_nav_url(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(FILE *out,void *data)
{
  struct btm tmp;

  assert(out  != NULL);
  assert(data != NULL);
  
  tmp = gd.previous;
  print_nav_url(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_current_url(FILE *out,void *data)
{
  struct btm tmp;

  assert(out != NULL);
  
  tmp = gd.now;
  print_nav_url(out,&tmp,MONTH);
}

/*********************************************************************/

static void print_nav_url(FILE *out,struct btm *date,int unit)
{
  assert(out  != NULL);
  assert(date != NULL);
  
  fprintf(out,"%s/",c_baseurl);
  print_nav_name(out,date,unit,'/');
}

/*******************************************************************/

static void print_nav_name(FILE *out,struct btm *date,int unit,char sep)
{
  assert(out  != NULL);
  assert(date != NULL);

  switch(unit)
  {
    case YEAR:
         fprintf(out,"%04d",date->year);
         break;
    case MONTH:
         fprintf(out,"%04d%c%02d",date->year,sep,date->month);
         break;
    case DAY:
         fprintf(out,"%04d%c%02d%c%02d",date->year,sep,date->month,sep,date->day);
         break;
    case PART:
         fprintf(out,"%04d%c%02d%c%02d.%d",date->year,sep,date->month,sep,date->day,date->part);
         break;
    default:
         assert(0);
         break;
  }
}

/********************************************************************/
 
static void cb_begin_year(FILE *out,void *data)
{
  assert(out != NULL);
  
  fprintf(out,"%04d",gd.begin.year);
}

/*******************************************************************/

static void cb_now_year(FILE *out,void *data)
{
  assert(out != NULL);
 
  fprintf(out,"%04d",gd.now.year);
}

/*******************************************************************/

static void cb_update_time(FILE *out,void *data)
{
  char buffer[BUFSIZ];
  
  assert(out != NULL);
  
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
  
  fputs(buffer,out);
}

/*******************************************************************/

static void cb_update_type(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(c_updatetype,out);
}

/*******************************************************************/

static void cb_robots_index(FILE *out,void *data)
{
  assert(out  != NULL);
  
  if ((gd.navunit == PART) || (gd.navunit == INDEX))
    fputs("index",out);
  else
    fputs("noindex",out);
}

/********************************************************************/

static void cb_comments(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.navunit != PART)
    return;
  
  generic_cb("comments",out,data);
}

/*******************************************************************/

static void cb_comments_body(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  FILE                 *in;
  char                  fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  sprintf(
  	fname,
  	"%04d/%02d/%02d/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
  
  in = fopen(fname,"r");
  if (in == NULL) return;
  fcopy(out,in);
  fclose(in);
}

/*******************************************************************/

static void cb_comments_filename(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);

  fprintf(
  	out,
  	"%04d/%02d/%02d/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
}

/*******************************************************************/

static void cb_comments_check(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char        fname[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);

  sprintf(
  	fname,
  	"%04d/%02d/%02d/%d.comments",
  	cbd->entry->when.year,
  	cbd->entry->when.month,
  	cbd->entry->when.day,
  	cbd->entry->when.part
  );
    
  if (access(fname,R_OK) < 0)
    fputs("No ",out);

  fputs("Comments",out);
}

/********************************************************************/

static void cb_edit(FILE *out,void *data)
{
  assert(out  != NULL);
  
  if (gd.f.edit == 0) return;
  generic_cb("edit",out,data);
}

/*********************************************************************/

static void cb_edit_author(FILE *out,void *data)
{
  char *name;
  
  assert(out  != NULL);
  assert(data != NULL);

  if (gd.req->origauthor != NULL)
    fputs(gd.req->origauthor,out);
  else
  {
    name = get_remote_user();
    fputs(name,out);
    free(name);
  }
}

/********************************************************************/

static void cb_edit_title(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.req->title != NULL)
    fputs(gd.req->title,out);
}

/********************************************************************/

static void cb_edit_date(FILE *out,void *data)
{
  time_t     now;
  struct tm *ptm;
  char       buffer[BUFSIZ];

  assert(out  != NULL);
  assert(data != NULL);

  if (gd.req->date != NULL)
    fputs(gd.req->date,out);
  else
  {
    now = time(NULL);
    ptm = localtime(&now);
    strftime(buffer,BUFSIZ,"%Y/%m/%d",ptm);
    fputs(buffer,out);
  }
}

/********************************************************************/

static void cb_edit_class(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.req->class)
    fputs(gd.req->class,out);
}

/********************************************************************/

static void cb_edit_email(FILE *out,void *data)
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_filter(FILE *out,void *data)
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_body(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  if (gd.req->origbody)
    fputs(gd.req->origbody,out);
}

/********************************************************************/

static void cb_xyzzy(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs("Nothing happens.",out);
}

/********************************************************************/

static void cb_date_day(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  print_nav_name(out,&entry->when,DAY,'-');
}

/*************************************************************************/

static void cb_date_day_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  print_nav_url(out,&entry->when,DAY);
}

/********************************************************************/

static void cb_date_day_normal(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  struct tm             day;
  char                  buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  tm_init(&day);
  entry       = cbd->entry;
  day.tm_year = entry->when.year - 1900;
  day.tm_mon  = entry->when.month - 1;
  day.tm_mday = entry->when.day;
  
  mktime(&day);
  
  strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&day);
  fputs(buffer,out);
}

/**********************************************************************/

static void cb_entry_cond_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);

  entry = cbd->entry;  
  
  if (btm_cmp_date(&entry->when,&cbd->last) != 0)
    generic_cb("entry.cond.date",out,data);
}

/**********************************************************************/

static void cb_entry_name(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  print_nav_name(out,&entry->when,PART,'-');
}

/***********************************************************************/

static void cb_overview(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
}

/***********************************************************************/

static void cb_overview_date(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs("DATE",out);
}

/***********************************************************************/

static void cb_overview_list(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs("<li>OVERVIEW</li>",out);
}

/**********************************************************************/

