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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>

#include <cgilib6/conf.h>
#include <cgilib6/htmltok.h>
#include <cgilib6/chunk.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "frontend.h"
#include "blogutil.h"
#include "conversion.h"
#include "globals.h"

#ifdef EMAIL_NOTIFY
#  include <gdbm.h>
#endif

/*****************************************************************/

static void cb_ad                     (FILE *,void *);
static void cb_ad_content             (FILE *,void *);
static void cb_atom_categories        (FILE *,void *);
static void cb_atom_category          (FILE *,void *);
static void cb_atom_entry             (FILE *,void *);
static void cb_begin_year             (FILE *,void *);
static void cb_blog_adtag             (FILE *,void *);
static void cb_blog_adtag_entity      (FILE *,void *);
static void cb_blog_author            (FILE *,void *);
static void cb_blog_author_email      (FILE *,void *);
static void cb_blog_class             (FILE *,void *);
static void cb_blog_description       (FILE *,void *);
static void cb_blog_name              (FILE *,void *);
static void cb_blog_script            (FILE *,void *);
static void cb_blog_title             (FILE *,void *);
static void cb_blog_url               (FILE *,void *);
static void cb_blog_url_home          (FILE *,void *);
static void cb_comments               (FILE *,void *);
static void cb_comments_body          (FILE *,void *);
static void cb_comments_check         (FILE *,void *);
static void cb_comments_filename      (FILE *,void *);
static void cb_cond_hr                (FILE *,void *);
static void cb_date_day               (FILE *,void *);
static void cb_date_day_normal        (FILE *,void *);
static void cb_date_day_url           (FILE *,void *);
static void cb_edit                   (FILE *,void *);
static void cb_edit_adtag             (FILE *,void *);
static void cb_edit_author            (FILE *,void *);
static void cb_edit_body              (FILE *,void *);
static void cb_edit_class             (FILE *,void *);
static void cb_edit_date              (FILE *,void *);
static void cb_edit_email             (FILE *,void *);
static void cb_edit_filter            (FILE *,void *);
static void cb_edit_status            (FILE *,void *);
static void cb_edit_title             (FILE *,void *);
static void cb_entry                  (FILE *,void *);
static void cb_entry_adtag            (FILE *,void *);
static void cb_entry_author           (FILE *,void *);
static void cb_entry_body             (FILE *,void *);
static void cb_entry_body_entified    (FILE *,void *);
static void cb_entry_body_jsonified   (FILE *,void *);
static void cb_entry_class            (FILE *,void *);
static void cb_entry_class_jsonified  (FILE *,void *);
static void cb_entry_cond_author      (FILE *,void *);
static void cb_entry_cond_date        (FILE *,void *);
static void cb_entry_date             (FILE *,void *);
static void cb_entry_description      (FILE *,void *);
static void cb_entry_id               (FILE *,void *);
static void cb_entry_linkdate         (FILE *,void *);
static void cb_entry_linkdated        (FILE *,void *);
static void cb_entry_name             (FILE *,void *);
static void cb_entry_path             (FILE *,void *);
static void cb_entry_pubdate          (FILE *,void *);
static void cb_entry_pubdatetime      (FILE *,void *);
static void cb_entry_status           (FILE *,void *);
static void cb_entry_title            (FILE *,void *);
static void cb_entry_url              (FILE *,void *);
static void cb_generator              (FILE *,void *);
static void cb_json_item              (FILE *,void *);
static void cb_navigation_bar         (FILE *,void *);
static void cb_navigation_bar_next    (FILE *,void *);
static void cb_navigation_bar_prev    (FILE *,void *);
static void cb_navigation_current     (FILE *,void *);
static void cb_navigation_current_url (FILE *,void *);
static void cb_navigation_first_title (FILE *,void *);
static void cb_navigation_first_url   (FILE *,void *);
static void cb_navigation_last_title  (FILE *,void *);
static void cb_navigation_last_url    (FILE *,void *);
static void cb_navigation_link        (FILE *,void *);
static void cb_navigation_link_next   (FILE *,void *);
static void cb_navigation_link_prev   (FILE *,void *);
static void cb_navigation_next_title  (FILE *,void *);
static void cb_navigation_next_url    (FILE *,void *);
static void cb_navigation_prev_title  (FILE *,void *);
static void cb_navigation_prev_url    (FILE *,void *);
static void cb_now_year               (FILE *,void *);
static void cb_overview               (FILE *,void *);
static void cb_overview_date          (FILE *,void *);
static void cb_overview_list          (FILE *,void *);
static void cb_request_url            (FILE *,void *);
static void cb_robots_index           (FILE *,void *);
static void cb_rss_item               (FILE *,void *);
static void cb_rss_item_url           (FILE *,void *);
static void cb_rss_pubdate            (FILE *,void *);
static void cb_rss_url                (FILE *,void *);
static void cb_update_time            (FILE *,void *);
static void cb_update_type            (FILE *,void *);
static void cb_xyzzy                  (FILE *,void *);

static void print_nav_url             (FILE *,struct btm const *,int);
static void print_nav_title           (FILE *,struct btm const *,int);
static void print_nav_name            (FILE *,struct btm const *,int,char);
static void fixup_uri                 (BlogEntry *,HtmlToken,char const *);
static void handle_aflinks            (HtmlToken,char const *);

/************************************************************************/

        /*--------------------------------
        ; the following table needs to
        ; be in alphabetical order
        ;--------------------------------*/
        
static struct chunk_callback const m_callbacks[] =
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
  { "blog.class"                , cb_blog_class                 } ,
  { "blog.description"          , cb_blog_description           } ,
  { "blog.name"                 , cb_blog_name                  } ,
  { "blog.script"               , cb_blog_script                } ,
  { "blog.title"                , cb_blog_title                 } ,
  { "blog.url"                  , cb_blog_url                   } ,
  { "blog.url.home"             , cb_blog_url_home              } ,
  { "comments"                  , cb_comments                   } ,
  { "comments.body"             , cb_comments_body              } ,
  { "comments.check"            , cb_comments_check             } ,
  { "comments.filename"         , cb_comments_filename          } ,
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
  { "entry.body.jsonified"      , cb_entry_body_jsonified       } ,
  { "entry.class"               , cb_entry_class                } ,
  { "entry.class.jsonified"     , cb_entry_class_jsonified      } ,
  { "entry.cond.author"         , cb_entry_cond_author          } ,
  { "entry.cond.date"           , cb_entry_cond_date            } ,
  { "entry.date"                , cb_entry_date                 } ,
  { "entry.description"         , cb_entry_description          } ,
  { "entry.id"                  , cb_entry_id                   } ,
  { "entry.linkdate"            , cb_entry_linkdate             } ,
  { "entry.linkdated"           , cb_entry_linkdated            } ,
  { "entry.name"                , cb_entry_name                 } ,
  { "entry.path"                , cb_entry_path                 } ,
  { "entry.pubdate"             , cb_entry_pubdate              } ,
  { "entry.pubdatetime"         , cb_entry_pubdatetime          } ,
  { "entry.status"              , cb_entry_status               } ,
  { "entry.title"               , cb_entry_title                } ,
  { "entry.url"                 , cb_entry_url                  } ,
  { "generator"                 , cb_generator                  } ,
  { "json.item"                 , cb_json_item                  } ,
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
  { "request.url"               , cb_request_url                } ,
  { "robots.index"              , cb_robots_index               } ,
  { "rss.item"                  , cb_rss_item                   } ,
  { "rss.item.url"              , cb_rss_item_url               } ,
  { "rss.pubdate"               , cb_rss_pubdate                } ,
  { "rss.url"                   , cb_rss_url                    } ,
  { "update.time"               , cb_update_time                } ,
  { "update.type"               , cb_update_type                } ,
  { "xyzzy"                     , cb_xyzzy                      } ,
};

static size_t const m_cbnum = sizeof(m_callbacks) / sizeof(struct chunk_callback);

/*************************************************************************/

void generic_cb(char const *which,FILE *out,void *data)
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

static void tm_init(struct tm *ptm)
{
  assert(ptm != NULL);
  
  memset(ptm,0,sizeof(struct tm));
  ptm->tm_isdst = -1;
}

/*********************************************************************/

static void cb_ad(FILE *out,void *data)
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

static void cb_blog_script(FILE *out,void *data __attribute__((unused)))
{
  char *script;
  
  assert(out != NULL);
  
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
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY);
}

/*************************************************************************/

static void cb_blog_url_home(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_fullbaseurl,out);
}

/**********************************************************************/

static void cb_blog_name(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_name,out);
}

/*********************************************************************/

static void cb_blog_title(FILE *out,void *data)
{
  if ((gd.navunit == UNIT_PART) && (data != NULL))
  {
    struct callback_data *cbd   = data;
    BlogEntry            *entry = (BlogEntry *)ListGetHead(&cbd->list);
    
    if (NodeValid(&entry->node) && entry->valid)
      fprintf(out,"%s - ",entry->title);
  }
  
  fputs(c_name,out);
}

/*********************************************************************/

static void cb_blog_author(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_author,out);
}

/********************************************************************/

static void cb_blog_author_email(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_email,out);
}

/**********************************************************************/

static void cb_blog_class(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_class,out);
}

/**********************************************************************/

static void cb_blog_description(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_description,out);
}

/*********************************************************************/

static void cb_blog_adtag(FILE *out,void *data)
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

static void cb_blog_adtag_entity(FILE *out,void *data)
{
  struct callback_data *cbd;
  FILE                 *entityout;
  char const          *tag;
  
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

static void cb_entry(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
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
    entry = (BlogEntry *)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry *)ListRemHead(&cbd->list)
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

static void cb_entry_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_PART);
}

/**********************************************************************/

static void cb_entry_id(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_DAY,'-');
}

/**********************************************************************/

static void cb_entry_linkdate(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_PART,'/');
}

/**********************************************************************/

static void cb_entry_linkdated(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_DAY,'-');
}

/**********************************************************************/

static void cb_entry_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY);
}

/*********************************************************************/

static void cb_entry_description(FILE *out,void *data)
{
  char const *msg = c_description;
  
  assert(out != NULL);
  
  if ((gd.navunit == UNIT_PART) && (data != NULL))
  {
    struct callback_data *cbd   = data;
    BlogEntry            *entry = (BlogEntry *)ListGetHead(&cbd->list);
    
    if (NodeValid(&entry->node) && entry->valid)
    {
      if (!empty_string(entry->status))
        msg = entry->status;
      else if (!empty_string(entry->title))
        msg = entry->title;
    }
  }
  
  fputs(msg,out);
}

/*********************************************************************/

static void cb_entry_pubdate(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  buffer[12];
  
  assert(out  != NULL);
  assert(data != NULL);
  assert(cbd->entry->valid);
  
  strftime(buffer,sizeof(buffer),"%F",localtime(&cbd->entry->timestamp));
  fputs(buffer,out);
}

/*********************************************************************/

static void cb_entry_pubdatetime(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  buffer[24];
  
  assert(out  != NULL);
  assert(data != NULL);
  assert(cbd->entry->valid);
  
  strftime(buffer,sizeof(buffer),"%FT%TZ",gmtime(&cbd->entry->timestamp));
  fputs(buffer,out);
}

/********************************************************************/

static void cb_entry_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->title,out);
}

/***********************************************************************/

static void cb_entry_class(FILE *out,void *data)
{
  char const *msg = c_class;
  
  assert(out  != NULL);
  
  if (data != NULL)
  {
    struct callback_data *cbd = data;
    
    if (cbd->entry != NULL)
      msg = cbd->entry->class;
    else
    {
      if (gd.navunit == UNIT_PART)
      {
        BlogEntry *entry = (BlogEntry *)ListGetHead(&cbd->list);
        if (NodeValid(&entry->node) && entry->valid)
          if (!empty_string(entry->class))
            msg = entry->class;
      }
    }
  }
  
  fputs(msg,out);
}

/***********************************************************************/

static void cb_entry_class_jsonified(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  String               *cats;
  size_t                num;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  cats = tag_split(&num,cbd->entry->class);
  for (size_t i = 0 ; i < num ; i++)
  {
    fputc('"',out);
    FILE *jout = fjson_encode_onwrite(out);
    if (jout)
    {
      fwrite(cats[i].d,cats[i].s,1,jout);
      fclose(jout);
    }
    fputc('"',out);
    if (i < num - 1)
      fputc(',',out);
  }
  
  free(cats);
}

/***********************************************************************/

static void cb_entry_status(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->entry->status,out);
}

/************************************************************************/

static void cb_entry_adtag(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs("Soon young Skywalker.  Soon.",out);
}

/**********************************************************************/

static void cb_entry_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->author,out);
}

/*************************************************************************/

static void handle_aflinks(HtmlToken token,char const *attrib)
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

static bool uri_scheme(char const *s)
{
  if (!isalpha(*s++)) return false;
  
  while(*s)
  {
    if (*s == ':')
      return true;
    
    if (isalpha(*s) || isdigit(*s) || (*s == '+') || (*s == '-') || (*s == '.'))
      s++;
    else
      return false;
  }
  
  return false;
}

/*************************************************************************/

static void fixup_uri(BlogEntry *entry,HtmlToken token,char const *attrib)
{
  struct pair *src;
  struct pair *np;
  
  assert(entry  != NULL);
  assert(entry->valid);
  assert(token  != NULL);
  assert(attrib != NULL);
  
  handle_aflinks(token,attrib);
  
  src = HtmlParseGetPair(token,attrib);
  if ((src != NULL) && !uri_scheme(src->value))
  {
    char        buffer[BUFSIZ];
    char const *baseurl;
    
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

static void cb_entry_body(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
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

static void cb_entry_body_entified(FILE *out,void *data)
{
  FILE *eout;
  
  assert(out != NULL);
  
  eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  cb_entry_body(eout,data);
  fclose(eout);
}

/*********************************************************************/

static void cb_entry_body_jsonified(FILE *out,void *data)
{
  FILE *jout;
  
  assert(out != NULL);
  
  jout = fjson_encode_onwrite(out);
  if (jout == NULL) return;
  cb_entry_body(jout,data);
  fclose(jout);
}

/**********************************************************************/

static void cb_cond_hr(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out != NULL);
  
  if (gd.f.navigation && (gd.navunit == UNIT_PART))
    return;
    
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  if (btm_cmp_date(&entry->when,&cbd->last) == 0)
  {
    if (entry->when.part != cbd->last.part)
      generic_cb("cond.hr",out,data);
  }
}

/*********************************************************************/

static void cb_rss_pubdate(FILE *out,void *data __attribute__((unused)))
{
  struct tm *ptm;
  char       buffer[BUFSIZ];
  
  assert(out != NULL);
  
  ptm = gmtime(&g_blog->tnow);
  strftime(buffer,sizeof(buffer),"%a, %d %b %Y %H:%M:%S GMT",ptm);
  fputs(buffer,out);
}

/********************************************************************/

static void cb_rss_url(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fprintf(out,"%s/",c_fullbaseurl);
}

/*******************************************************************/

static void cb_rss_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  for
  (
    entry = (BlogEntry *)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry *)ListRemHead(&cbd->list)
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

static void cb_rss_item_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fprintf(out,"%s",c_fullbaseurl);
  print_nav_url(out,&cbd->entry->when,UNIT_PART);
}

/********************************************************************/

static void cb_atom_entry(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /*----------------------------------------------------------------------
  ; XXX---this code is identical to cb_rss_item(), except for which template
  ; is being used.  Could possibly use some refactoring here ...
  ;-----------------------------------------------------------------------*/
  
  for
  (
    entry = (BlogEntry *)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry *)ListRemHead(&cbd->list)
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
  FILE *eout;
  
  assert(out != NULL);
  
  eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  fputs(data,eout);
  fclose(eout);
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
  assert(out != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.link.next",out,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(FILE *out,void *data)
{
  assert(out != NULL);
  
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
  assert(out != NULL);
  
  if (gd.f.navnext == false) return;
  generic_cb("navigation.bar.next",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(FILE *out,void *data)
{
  assert(out != NULL);
  
  if (gd.f.navprev == false) return;
  generic_cb("navigation.bar.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_current(FILE *out,void *data)
{
  assert(out != NULL);
  generic_cb("navigation.current",out,data);
}

/********************************************************************/

static void cb_navigation_first_url(FILE *out,void *data __attribute__((unused)))
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

static void cb_navigation_first_title(FILE *out,void *data __attribute__((unused)))
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

static void cb_navigation_last_url(FILE *out,void *data __attribute__((unused)))
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

static void cb_navigation_last_title(FILE *out,void *data __attribute__((unused)))
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

static void cb_navigation_next_title(FILE *out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.next;
  print_nav_title(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_prev_title(FILE *out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.previous;
  print_nav_title(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_next_url(FILE *out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.next;
  print_nav_url(out,&tmp,gd.navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(FILE *out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = gd.previous;
  print_nav_url(out,&tmp,gd.navunit);
}

/********************************************************************/

static void cb_navigation_current_url(FILE *out,void *data __attribute__((unused)))
{
  struct btm tmp;
  
  assert(out != NULL);
  
  tmp = g_blog->last;
  print_nav_url(out,&tmp,UNIT_MONTH);
}

/*********************************************************************/

static void print_nav_title(FILE *out,struct btm const *date,int unit)
{
  BlogEntry *entry;
  struct tm  stm;
  char       buffer[BUFSIZ];
  
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
         strftime(buffer,sizeof(buffer),"%B %Y",&stm);
         fputs(buffer,out);
         break;
    case UNIT_DAY:
         tm_init(&stm);
         stm.tm_year = date->year - 1900;
         stm.tm_mon  = date->month - 1;
         stm.tm_mday = date->day;
         mktime(&stm);
         strftime(buffer,sizeof(buffer),"%A, %B %d, %Y",&stm);
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

static void print_nav_url(FILE *out,struct btm const *date,int unit)
{
  assert(out  != NULL);
  assert(date != NULL);
  
  fprintf(out,"%s/",c_baseurl);
  print_nav_name(out,date,unit,'/');
}

/*******************************************************************/

static void print_nav_name(FILE *out,struct btm const *date,int unit,char sep)
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

static void cb_begin_year(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fprintf(out,"%04d",g_blog->first.year);
}

/*******************************************************************/

static void cb_now_year(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fprintf(out,"%04d",g_blog->now.year);
}

/*******************************************************************/

static void cb_update_time(FILE *out,void *data __attribute__((unused)))
{
  char tmpbuf[24];
  
  assert(out != NULL);
  
  strftime(tmpbuf,sizeof(tmpbuf),"%FT%TZ",gmtime(&g_blog->tnow));
  fputs(tmpbuf,out);
}

/*******************************************************************/

static void cb_update_type(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs(c_updatetype,out);
}

/*******************************************************************/

static void cb_robots_index(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.navunit == UNIT_PART)
    fputs("index",out);
  else
    fputs("noindex",out);
}

/********************************************************************/

static void cb_comments(FILE *out,void *data)
{
  assert(out != NULL);
  
  if (gd.navunit != UNIT_PART)
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
  char                  fname[BUFSIZ];
  
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
  assert(out != NULL);
  
  if (gd.f.edit == 0) return;
  generic_cb("edit",out,data);
}

/*********************************************************************/

static void cb_edit_adtag(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.adtag != NULL)
    fputs(gd.req.adtag,out);
}

/*********************************************************************/

static void cb_edit_author(FILE *out,void *data __attribute__((unused)))
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

static void cb_edit_title(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.title != NULL)
    fputs(gd.req.title,out);
}

/********************************************************************/

static void cb_edit_status(FILE *out,void *data __attribute__ ((unused)))
{
  assert(out != NULL);
  
  if (gd.req.status != NULL)
    fputs(gd.req.status,out);
}

/**********************************************************************/

static void cb_edit_date(FILE *out,void *data __attribute__((unused)))
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
    strftime(buffer,sizeof(buffer),"%Y/%m/%d",ptm);
    fputs(buffer,out);
  }
}

/********************************************************************/

static void cb_edit_class(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.class)
    fputs(gd.req.class,out);
}

/********************************************************************/

static void cb_edit_email(FILE *out __attribute__((unused)),void *data __attribute__((unused)))
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_filter(FILE *out __attribute__((unused)),void *data __attribute__((unused)))
{
  /* XXX */
}

/*******************************************************************/

static void cb_edit_body(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  
  if (gd.req.origbody)
    fputs(gd.req.origbody,out);
}

/********************************************************************/

static void cb_xyzzy(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs("Nothing happens.",out);
}

/********************************************************************/

static void cb_date_day(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_DAY,'-');
}

/*************************************************************************/

static void cb_date_day_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_DAY);
}

/********************************************************************/

static void cb_date_day_normal(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
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
  
  strftime(buffer,sizeof(buffer),"%A, %B %d, %Y",&day);
  fputs(buffer,out);
}

/**********************************************************************/

static void cb_entry_cond_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  
  if (strcmp(c_author,cbd->entry->author) != 0)
    generic_cb("entry.cond.author",out,data);
}

/*********************************************************************/
static void cb_entry_cond_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  
  if (btm_cmp_date(&entry->when,&cbd->last) != 0)
    generic_cb("entry.cond.date",out,data);
}

/**********************************************************************/

static void cb_entry_name(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_PART,'-');
}

/***********************************************************************/

static void cb_entry_path(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_name(out,&entry->when,UNIT_PART,'/');
}

/**********************************************************************/

static void cb_overview(FILE *out __attribute__((unused)),void *data __attribute__((unused)))
{
  assert(out  != NULL);
  assert(data != NULL);
}

/***********************************************************************/

static void cb_overview_date(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs("DATE",out);
}

/***********************************************************************/

static void cb_overview_list(FILE *out,void *data __attribute__((unused)))
{
  assert(out != NULL);
  fputs("OVERVIEW",out);
}

/**********************************************************************/

static void cb_request_url(FILE *out,void *data __attribute__((unused)))
{
  char *tum;
  
  assert(out != NULL);
  
  tum = tumbler_canonical(&gd.req.tumbler);
  fprintf(out,"%s/%s",c_fullbaseurl,tum);
  free(tum);
}

/***********************************************************************/

static void cb_generator(FILE *out,void *data __attribute__((unused)))
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

static void cb_json_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  char const           *sep = "";
  
  assert(out  != NULL);
  assert(data != NULL);
  
  /*----------------------------------------------------------------------
  ; XXX---this code is identical to cb_rss_item(), except for which template
  ; is being used.  Could possibly use some refactoring here ...
  ;-----------------------------------------------------------------------*/
  
  for
  (
    entry = (BlogEntry *)ListRemHead(&cbd->list);
    NodeValid(&entry->node);
    entry = (BlogEntry *)ListRemHead(&cbd->list)
  )
  {
    assert(entry->valid);
    cbd->entry = entry;
    fputs(sep,out);
    generic_cb("item",out,data);
    cbd->last = entry->when;
    BlogEntryFree(entry);
    sep = ",";
  }
  
  cbd->entry = NULL;
}

/*********************************************************************/
