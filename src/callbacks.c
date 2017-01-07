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

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>

#include <cgilib6/conf.h>
#include <cgilib6/htmltok.h>
#include <cgilib6/chunk.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "frontend.h"
#include "fix.h"
#include "blogutil.h"
#include "globals.h"

#ifdef EMAIL_NOTIFY
#  include <gdbm.h>
#endif

/*****************************************************************/

static void     cb_ad                           (FILE *const,void *);
static void     cb_ad_content                   (FILE *const,void *);
static void     cb_atom_categories              (FILE *const,void *);
static void     cb_atom_category                (FILE *const,void *);
static void     cb_atom_entry                   (FILE *const,void *);
static void     cb_begin_year                   (FILE *const,void *);
static void     cb_blog_adtag                   (FILE *const,void *);
static void     cb_blog_adtag_entity            (FILE *const,void *);
static void     cb_blog_author                  (FILE *const,void *);
static void     cb_blog_author_email            (FILE *const,void *);
static void     cb_blog_name                    (FILE *const,void *);
static void     cb_blog_script                  (FILE *const,void *);
static void     cb_blog_url                     (FILE *const,void *);
static void     cb_blog_url_home                (FILE *const,void *);
static void     cb_comments                     (FILE *const,void *);
static void     cb_comments_body                (FILE *const,void *);
static void     cb_comments_check               (FILE *const,void *);
static void     cb_comments_filename            (FILE *const,void *);
static void     cb_cond_blog_title              (FILE *const,void *);
static void     cb_cond_hr                      (FILE *const,void *);
static void     cb_date_day                     (FILE *const,void *);
static void     cb_date_day_normal              (FILE *const,void *);
static void     cb_date_day_url                 (FILE *const,void *);
static void     cb_edit                         (FILE *const,void *);
static void     cb_edit_adtag                   (FILE *const,void *);
static void     cb_edit_author                  (FILE *const,void *);
static void     cb_edit_body                    (FILE *const,void *);
static void     cb_edit_class                   (FILE *const,void *);
static void     cb_edit_date                    (FILE *const,void *);
static void     cb_edit_email                   (FILE *const,void *);
static void     cb_edit_filter                  (FILE *const,void *);
static void     cb_edit_status                  (FILE *const,void *);
static void     cb_edit_title                   (FILE *const,void *);
static void     cb_entry                        (FILE *const,void *);
static void     cb_entry_adtag                  (FILE *const,void *);
static void     cb_entry_author                 (FILE *const,void *);
static void     cb_entry_body                   (FILE *const,void *);
static void     cb_entry_body_entified          (FILE *const,void *);
static void     cb_entry_class                  (FILE *const,void *);
static void     cb_entry_cond_author            (FILE *const,void *);
static void     cb_entry_cond_date              (FILE *const,void *);
static void     cb_entry_date                   (FILE *const,void *);
static void     cb_entry_id                     (FILE *const,void *);
static void     cb_entry_name                   (FILE *const,void *);
static void     cb_entry_pubdate                (FILE *const,void *);
static void     cb_entry_status                 (FILE *const,void *);
static void     cb_entry_title                  (FILE *const,void *);
static void     cb_entry_url                    (FILE *const,void *);
static void     cb_generator                    (FILE *const,void *);
static void     cb_navigation_bar               (FILE *const,void *);
static void     cb_navigation_bar_next          (FILE *const,void *);
static void     cb_navigation_bar_prev          (FILE *const,void *);
static void     cb_navigation_current           (FILE *const,void *);
static void     cb_navigation_current_url       (FILE *const,void *);
static void     cb_navigation_first_title       (FILE *const,void *);
static void     cb_navigation_first_url         (FILE *const,void *);
static void     cb_navigation_last_title        (FILE *const,void *);
static void     cb_navigation_last_url          (FILE *const,void *);
static void     cb_navigation_link              (FILE *const,void *);
static void     cb_navigation_link_next         (FILE *const,void *);
static void     cb_navigation_link_prev         (FILE *const,void *);
static void     cb_navigation_next_title        (FILE *const,void *);
static void     cb_navigation_next_url          (FILE *const,void *);
static void     cb_navigation_prev_title        (FILE *const,void *);
static void     cb_navigation_prev_url          (FILE *const,void *);
static void     cb_now_year                     (FILE *const,void *);
static void     cb_overview                     (FILE *const,void *);
static void     cb_overview_date                (FILE *const,void *);
static void     cb_overview_list                (FILE *const,void *);
static void     cb_robots_index                 (FILE *const,void *);
static void     cb_rss_item                     (FILE *const,void *);
static void     cb_rss_item_url                 (FILE *const,void *);
static void     cb_rss_pubdate                  (FILE *const,void *);
static void     cb_rss_url                      (FILE *const,void *);
static void     cb_update_time                  (FILE *const,void *);
static void     cb_update_type                  (FILE *const,void *);
static void     cb_xyzzy                        (FILE *const,void *);

static void     print_nav_url   (FILE *const,const struct btm *const,int);
static void     print_nav_title (FILE *const,const struct btm *const,int);
static void     print_nav_name  (FILE *const,const struct btm *const,int,char);
static void     fixup_uri       (BlogEntry,HtmlToken,const char *);
static void     handle_aflinks  (HtmlToken,const char *);

/************************************************************************/

        /*--------------------------------
        ; the following table needs to
        ; be in alphabetical order
        ;--------------------------------*/
        
static const struct chunk_callback  m_callbacks[] =
{
  { "ad"                        , cb_ad                         } ,
  { "ad.content"                , cb_ad_content                 } ,
  { "atom.categories"           , cb_atom_categories            } ,
  { "atom.category"             , cb_atom_category              } ,
  { "atom.entry"                , cb_atom_entry                 } ,
  { "begin.year"                , cb_begin_year                 } ,
  { "blog.adtag"                , cb_blog_adtag                 } ,
  { "blog.adtag.entity"         , cb_blog_adtag_entity          } ,
  { "blog.author"               , cb_blog_author                } ,
  { "blog.author.email"         , cb_blog_author_email          } ,
  { "blog.name"                 , cb_blog_name                  } ,
  { "blog.script"               , cb_blog_script                } ,
  { "blog.url"                  , cb_blog_url                   } ,
  { "blog.url.home"             , cb_blog_url_home              } ,
  { "comments"                  , cb_comments                   } ,
  { "comments.body"             , cb_comments_body              } ,
  { "comments.check"            , cb_comments_check             } ,
  { "comments.filename"         , cb_comments_filename          } ,
  { "cond.blog.title"           , cb_cond_blog_title            } ,
  { "cond.hr"                   , cb_cond_hr                    } ,
  { "date.day"                  , cb_date_day                   } ,
  { "date.day.normal"           , cb_date_day_normal            } ,
  { "date.day.url"              , cb_date_day_url               } ,
  { "edit"                      , cb_edit                       } ,
  { "edit.adtag"                , cb_edit_adtag                 } ,
  { "edit.author"               , cb_edit_author                } ,
  { "edit.body"                 , cb_edit_body                  } ,
  { "edit.class"                , cb_edit_class                 } ,
  { "edit.date"                 , cb_edit_date                  } ,
  { "edit.email"                , cb_edit_email                 } ,
  { "edit.filter"               , cb_edit_filter                } ,
  { "edit.status"               , cb_edit_status                } ,
  { "edit.title"                , cb_edit_title                 } ,
  { "entry"                     , cb_entry                      } ,
  { "entry.adtag"               , cb_entry_adtag                } ,
  { "entry.author"              , cb_entry_author               } ,
  { "entry.body"                , cb_entry_body                 } ,
  { "entry.body.entified"       , cb_entry_body_entified        } ,
  { "entry.class"               , cb_entry_class                } ,
  { "entry.cond.author"         , cb_entry_cond_author          } ,
  { "entry.cond.date"           , cb_entry_cond_date            } ,
  { "entry.date"                , cb_entry_date                 } ,
  { "entry.id"                  , cb_entry_id                   } ,
  { "entry.name"                , cb_entry_name                 } ,
  { "entry.pubdate"             , cb_entry_pubdate              } ,
  { "entry.status"              , cb_entry_status               } ,
  { "entry.title"               , cb_entry_title                } ,
  { "entry.url"                 , cb_entry_url                  } ,
  { "generator"                 , cb_generator                  } ,
  { "navigation.bar"            , cb_navigation_bar             } ,
  { "navigation.bar.next"       , cb_navigation_bar_next        } ,
  { "navigation.bar.prev"       , cb_navigation_bar_prev        } ,
  { "navigation.current"        , cb_navigation_current         } ,
  { "navigation.current.url"    , cb_navigation_current_url     } ,
  { "navigation.first.title"    , cb_navigation_first_title     } ,
  { "navigation.first.url"      , cb_navigation_first_url       } ,
  { "navigation.last.title"     , cb_navigation_last_title      } ,
  { "navigation.last.url"       , cb_navigation_last_url        } ,
  { "navigation.link"           , cb_navigation_link            } ,
  { "navigation.link.next"      , cb_navigation_link_next       } ,
  { "navigation.link.prev"      , cb_navigation_link_prev       } ,
  { "navigation.next.title"     , cb_navigation_next_title      } ,
  { "navigation.next.url"       , cb_navigation_next_url        } ,
  { "navigation.prev.title"     , cb_navigation_prev_title      } ,
  { "navigation.prev.url"       , cb_navigation_prev_url        } ,
  { "now.year"                  , cb_now_year                   } ,
  { "overview"                  , cb_overview                   } ,
  { "overview.date"             , cb_overview_date              } ,
  { "overview.list"             , cb_overview_list              } ,
  { "robots.index"              , cb_robots_index               } ,
  { "rss.item"                  , cb_rss_item                   } ,
  { "rss.item.url"              , cb_rss_item_url               } ,
  { "rss.pubdate"               , cb_rss_pubdate                } ,
  { "rss.url"                   , cb_rss_url                    } ,
  { "update.time"               , cb_update_time                } ,
  { "update.type"               , cb_update_type                } ,
  { "xyzzy"                     , cb_xyzzy                      }
};

static const size_t m_cbnum = sizeof(m_callbacks) / sizeof(struct chunk_callback);

/*************************************************************************/

void generic_cb(const char *const which,FILE *const out,void *data)
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

static void cb_ad(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  char                  fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  
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

static void cb_ad_content(FILE *const out,void *data)
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

static void cb_blog_script(FILE *const out,void *data __attribute__((unused)))
{
  char *script;
  
  assert(out != NULL);
  
  script = getenv("SCRIPT_NAME");
  if (script)
    fputs(script,out);
}

/**********************************************************************/

static void cb_blog_url(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY);
}

/*************************************************************************/

static void cb_blog_url_home(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs(c_fullbaseurl,out);
}

/**********************************************************************/

static void cb_blog_name(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs(c_name,out);
}

/*********************************************************************/

static void cb_blog_author(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs(c_author,out);
}

/********************************************************************/

static void cb_blog_author_email(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs(c_email,out);
}

/*********************************************************************/

static void cb_blog_adtag(FILE *const out,void *data)
{
  struct callback_data *cbd;
  char                 *tag;
  
  assert(out != NULL);
  
  if (data == NULL)
    tag = UrlEncodeString(c_adtag);
  else
  {
    cbd = data;
    tag = UrlEncodeString(cbd->adtag);
  }
  
  fputs(tag,out);
  free(tag);
}

/*********************************************************************/

static void cb_blog_adtag_entity(FILE *const out,void *data)
{
  struct callback_data *cbd;
  FILE                 *entityout;
  const char           *tag;
  
  assert(out != NULL);
  
  if (data == NULL)
    tag = c_adtag;
  else
  {
    cbd = data;
    tag = cbd->adtag;
  }
  
  entityout = fentity_encode_onwrite(out);
  if (entityout == NULL) return;
  fputs(tag,entityout);
  fclose(entityout);
}

/********************************************************************/

static void cb_entry(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out != NULL);
  
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
    assert(entry->valid);
    cbd->entry = entry;
    generic_cb("entry",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/**********************************************************************/

static void cb_entry_url(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_PART);
}

/**********************************************************************/

static void cb_entry_id(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_DAY,'-');
}

/**********************************************************************/

static void cb_entry_date(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY);
}

/*********************************************************************/

static void cb_entry_pubdate(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  time_t                ts;
  struct tm            *ptm;
  char                  buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
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

static void cb_entry_title(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->title,out);
}

/***********************************************************************/

static void cb_entry_class(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->entry->class,out);
}

/***********************************************************************/

static void cb_entry_status(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->entry->status,out);
}

/************************************************************************/

static void cb_entry_adtag(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs("Soon young Skywalker.  Soon.",out);
}

/**********************************************************************/

static void cb_entry_author(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->author,out);
}

/*************************************************************************/

static void handle_aflinks(HtmlToken token,const char *attrib)
{
  struct pair *src;
  
  assert(token  != NULL);
  assert(attrib != NULL);
  
  src = HtmlParseGetPair(token,attrib);
  if (src != NULL)
  {
    for (size_t i = 0 ; i < c_numaflinks ; i++)
    {
      if (strncmp(src->value,c_aflinks[i].proto,c_aflinks[i].psize) == 0)
      {
        char buffer[BUFSIZ];
        struct pair *np;
        
        snprintf(
                buffer,
                sizeof(buffer),
                c_aflinks[i].format,
                &src->value[c_aflinks[i].psize + 1]
        );
        np = PairCreate(attrib,buffer);
        NodeInsert(&src->node,&np->node);
        NodeRemove(&src->node);
        PairFree(src);
        return;
      }
    }
  }
}

/*************************************************************************/

static void fixup_uri(BlogEntry entry,HtmlToken token,const char *attrib)
{
  struct pair *src;
  struct pair *np;
  
  assert(entry  != NULL);
  assert(entry->valid);
  assert(token  != NULL);
  assert(attrib != NULL);
  
  handle_aflinks(token,attrib);
  
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
       && (strncmp(src->value,"file:",5)        != 0)
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
    ; all this to reassign the value, without ``knowing'' how the pair stuff
    ; actually works.  Perhaps add a PairReValue() call or something?
    ;
    ; Anyway, check to see if baseurl ends in a '/', and if not, we then add
    ; one, otherwise, don't.
    ;
    ; If baseurl ends in a '/', and src->value does not start with a '/', we
    ; end up with a double '//'.  We need to make sure we canonicalize all
    ; partial urls to either end in a '/', or not end in a '/'.
    ;----------------------------------------------------------*/
    
    if (src->value[0] == '/')
      sprintf(buffer,"%s%s",baseurl,src->value);
    else if (isdigit(src->value[0]))
      sprintf(buffer,"%s/%s",baseurl,src->value);
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
    
    np = PairCreate(attrib,buffer);
    NodeInsert(&src->node,&np->node);
    NodeRemove(&src->node);
    PairFree(src);
  }
}

/***************************************************************/

static void cb_entry_body(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  FILE                 *in;
  HtmlToken             token;
  int                   t;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  in    = fmemopen(entry->body,strlen(entry->body),"r");
  if (in == NULL) return;
  
  token = HtmlParseNew(in);
  
  if (token == NULL)
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

static void cb_entry_body_entified(FILE *const out,void *data)
{
  FILE *eout;
  
  assert(out != NULL);
  
  eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  cb_entry_body(eout,data);
  fclose(eout);
}

/*********************************************************************/

static void cb_cond_hr(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out != NULL);
  
  if (gd.f.navigation && (gd.navunit == UNIT_PART))
    return;
    
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  if (btm_cmp_date(&entry->when,&cbd->last) == 0)
  {
    if (entry->when.part != cbd->last.part)
      fputs("<hr class=\"next\">",out);
  }
}

/*********************************************************************/

static void cb_cond_blog_title(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out != NULL);
  
  if (gd.navunit == UNIT_PART)
  {
    assert(data != NULL);
    entry = (BlogEntry)ListGetHead(&cbd->list);
    if (NodeValid(&entry->node) && entry->valid)
      fprintf(out,"%s - ",entry->title);
  }
}

/*********************************************************************/

static void cb_rss_pubdate(FILE *const out,void *data __attribute__((unused)))
{
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  assert(out != NULL);
  
  ptm = gmtime(&g_blog->tnow);
  strftime(buffer,BUFSIZ,"%a, %d %b %Y %H:%M:%S GMT",ptm);
  fputs(buffer,out);
}

/********************************************************************/

static void cb_rss_url(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fprintf(out,"%s/",c_fullbaseurl);
}

/*******************************************************************/

static void cb_rss_item(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  for
  (
    entry = (BlogEntry)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(&cbd->list)
  )
  {
    assert(entry->valid);
    cbd->entry = entry;
    generic_cb("item",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/******************************************************************/

static void cb_rss_item_url(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fprintf(out,"%s",c_fullbaseurl);
  print_nav_url(out,&cbd->entry->when,UNIT_PART);
}

/********************************************************************/

static void cb_atom_entry(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /*----------------------------------------------------------------------
  ; XXX---this code is identical to cb_rss_item(), except for which template
  ; is being used.  Could possibly use some refactoring here ...
  ;-----------------------------------------------------------------------*/
  
  for
  (
    entry = (BlogEntry)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(&cbd->list)
  )
  {
    assert(entry->valid);
    cbd->entry = entry;
    generic_cb("entry",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
  }
  
  cbd->entry = NULL;
}

/************************************************************************/

static void cb_atom_categories(FILE *const out,void *data)
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

static void cb_atom_category(FILE *const out,void *data)
{
  FILE *eout;
  
  assert(out != NULL);
  
  eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  fputs(data,eout);
  fclose(eout);
}

/****************************************************************/

static void cb_navigation_link(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navigation == false) return;
  generic_cb("navigation.link",out,data);
}

/********************************************************************/

static void cb_navigation_link_next(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.link.next",out,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navprev == false) return;
  generic_cb("navigation.link.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_bar(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navigation == false) return;
  generic_cb("navigation.bar",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_next(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.bar.next",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navprev == false) return;
  generic_cb("navigation.bar.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_current(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.navunit != UNIT_INDEX) return;
  generic_cb("navigation.current",out,data);
}

/********************************************************************/

static void cb_navigation_first_url(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->first;
  if (gd.f.navigation == false)
    print_nav_url(out,&tmp,UNIT_PART);
  else
    print_nav_url(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_first_title(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->first;
  if (gd.f.navigation == false)
    print_nav_title(out,&tmp,UNIT_PART);
  else
    print_nav_title(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_last_url(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->last;
  if (gd.f.navigation == false)
    print_nav_url(out,&tmp,UNIT_PART);
  else
    print_nav_url(out,&tmp,gd.navunit);
}

/******************************************************************/

static void cb_navigation_last_title(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->last;
  if (gd.f.navigation == false)
    print_nav_title(out,&tmp,UNIT_PART);
  else
    print_nav_title(out,&tmp,gd.navunit);
}

/*****************************************************************/

static void cb_navigation_next_title(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.next;
  print_nav_title(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_prev_title(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.previous;
  print_nav_title(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_next_url(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.next;
  print_nav_url(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.previous;
  print_nav_url(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_current_url(FILE *const out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->now;
  print_nav_url(out,&tmp,UNIT_MONTH);
}

/*********************************************************************/

static void print_nav_title(FILE *const out,const struct btm *const date,int unit)
{
  BlogEntry entry;
  struct tm stm;
  char      buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(date != NULL);
  
  switch(unit)
  {
    case UNIT_YEAR:
         fprintf(out,"%04d",date->year);
         break;
    case UNIT_MONTH:
         tm_init(&stm);
         stm.tm_year = date->year - 1900;
         stm.tm_mon  = date->month;
         mktime(&stm);
         strftime(buffer,BUFSIZ,"%B %Y",&stm);
         fputs(buffer,out);
         break;
    case UNIT_DAY:
         tm_init(&stm);
         stm.tm_year = date->year - 1900;
         stm.tm_mon  = date->month - 1;
         stm.tm_mday = date->day;
         mktime(&stm);
         strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&stm);
         fputs(buffer,out);
         break;
    case UNIT_PART:
         entry = BlogEntryRead(g_blog,date);
         if (entry)
         {
           assert(entry->valid);
           fputs(entry->title,out);
           BlogEntryFree(entry);
         }
         break;
    default:
         assert(0);
         break;
  }
}

/**********************************************************************/

static void print_nav_url(FILE *const out,const struct btm *const date,int unit)
{
  assert(out  != NULL);
  assert(date != NULL);
  
  fprintf(out,"%s/",c_baseurl);
  print_nav_name(out,date,unit,'/');
}

/*******************************************************************/

static void print_nav_name(FILE *const out,const struct btm *const date,int unit,char sep)
{
  assert(out  != NULL);
  assert(date != NULL);
  
  switch(unit)
  {
    case UNIT_YEAR:
         fprintf(out,"%04d",date->year);
         break;
    case UNIT_MONTH:
         fprintf(out,"%04d%c%02d",date->year,sep,date->month);
         break;
    case UNIT_DAY:
         fprintf(out,"%04d%c%02d%c%02d",date->year,sep,date->month,sep,date->day);
         break;
    case UNIT_PART:
         fprintf(out,"%04d%c%02d%c%02d.%d",date->year,sep,date->month,sep,date->day,date->part);
         break;
    default:
         assert(0);
         break;
  }
}

/********************************************************************/

static void cb_begin_year(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fprintf(out,"%04d",g_blog->first.year);
}

/*******************************************************************/

static void cb_now_year(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fprintf(out,"%04d",g_blog->now.year);
}

/*******************************************************************/

static void cb_update_time(FILE *const out,void *data __attribute__((unused)))
{
  char tmpbuf[20];
  
  assert(out != NULL);
  
  strftime(tmpbuf,sizeof(tmpbuf),"%Y-%m-%dT%H:%M:%S",localtime(&g_blog->tnow));
  fprintf(out,"%s%+03d:%02d",tmpbuf,c_tzhour,c_tzmin);
}

/*******************************************************************/

static void cb_update_type(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs(c_updatetype,out);
}

/*******************************************************************/

static void cb_robots_index(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if ((gd.navunit == UNIT_PART) || (gd.navunit == UNIT_INDEX))
    fputs("index",out);
  else
    fputs("noindex",out);
}

/********************************************************************/

static void cb_comments(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.navunit != UNIT_PART)
    return;
    
  generic_cb("comments",out,data);
}

/*******************************************************************/

static void cb_comments_body(FILE *const out,void *data)
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

static void cb_comments_filename(FILE *const out,void *data)
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

static void cb_comments_check(FILE *const out,void *data)
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

static void cb_edit(FILE *const out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.edit == 0) return;
  generic_cb("edit",out,data);
}

/*********************************************************************/

static void cb_edit_adtag(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.adtag != NULL)
    fputs(gd.req.adtag,out);
}

/*********************************************************************/

static void cb_edit_author(FILE *const out,void *data __attribute__((unused)))
{
  char *name;
  
  assert(out != NULL);
  
  if (gd.req.origauthor != NULL)
    fputs(gd.req.origauthor,out);
  else
  {
    name = get_remote_user();
    fputs(name,out);
    free(name);
  }
}

/********************************************************************/

static void cb_edit_title(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.title != NULL)
    fputs(gd.req.title,out);
}

/********************************************************************/

static void cb_edit_status(FILE *const out,void *data __attribute__ ((unused)))
{
  assert(out != NULL);
  
  if (gd.req.status != NULL)
    fputs(gd.req.status,out);
}

/**********************************************************************/

static void cb_edit_date(FILE *const out,void *data __attribute__((unused)))
{
  time_t     now;
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  assert(out != NULL);
  
  if (gd.req.date != NULL)
    fputs(gd.req.date,out);
  else
  {
    now = time(NULL);
    ptm = localtime(&now);
    strftime(buffer,BUFSIZ,"%Y/%m/%d",ptm);
    fputs(buffer,out);
  }
}

/********************************************************************/

static void cb_edit_class(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.class)
    fputs(gd.req.class,out);
}

/********************************************************************/

static void cb_edit_email(FILE *const out __attribute__((unused)),void *data __attribute__((unused)))
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_filter(FILE *const out __attribute__((unused)),void *data __attribute__((unused)))
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_body(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.origbody)
    fputs(gd.req.origbody,out);
}

/********************************************************************/

static void cb_xyzzy(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs("Nothing happens.",out);
}

/********************************************************************/

static void cb_date_day(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_DAY,'-');
}

/*************************************************************************/

static void cb_date_day_url(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_DAY);
}

/********************************************************************/

static void cb_date_day_normal(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  struct tm             day;
  char                  buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  tm_init(&day);
  entry       = cbd->entry;
  assert(entry->valid);
  day.tm_year = entry->when.year - 1900;
  day.tm_mon  = entry->when.month - 1;
  day.tm_mday = entry->when.day;
  
  mktime(&day);
  
  strftime(buffer,BUFSIZ,"%A, %B %d, %Y",&day);
  fputs(buffer,out);
}

/**********************************************************************/

static void cb_entry_cond_author(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  
  if (strcmp(c_author,cbd->entry->author) != 0)
    generic_cb("entry.cond.author",out,data);
}

/*********************************************************************/
static void cb_entry_cond_date(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  
  if (btm_cmp_date(&entry->when,&cbd->last) != 0)
    generic_cb("entry.cond.date",out,data);
}

/**********************************************************************/

static void cb_entry_name(FILE *const out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry             entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_PART,'-');
}

/***********************************************************************/

static void cb_overview(FILE *const out __attribute__((unused)),void *data __attribute__((unused)))
{
  assert(out  != NULL);
  assert(data != NULL);
}

/***********************************************************************/

static void cb_overview_date(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs("DATE",out);
}

/***********************************************************************/

static void cb_overview_list(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  fputs("<li>OVERVIEW</li>",out);
}

/**********************************************************************/

static void cb_generator(FILE *const out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
#ifdef EMAIL_NOTIFY
  fprintf(
    out,
    "mod_blog %s, %s, %s, %s"
    "",
    PROG_VERSION,
    cgilib_version,
    LUA_RELEASE,
    gdbm_version
  );
#else
  fprintf(
    out,
    "mod_blog %s, %s, %s"
    "",
    PROG_VERSION,
    cgilib_version,
    LUA_RELEASE
  );
#endif
}

/*********************************************************************/
