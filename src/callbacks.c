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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <gdbm.h>
#include <syslog.h>

#include <cgilib8/conf.h>
#include <cgilib8/htmltok.h>
#include <cgilib8/chunk.h>
#include <cgilib8/util.h>

#include "backend.h"
#include "blogutil.h"
#include "conversion.h"

/*****************************************************************/

static void tm_init(struct tm *ptm)
{
  assert(ptm != NULL);
  
  memset(ptm,0,sizeof(struct tm));
  ptm->tm_isdst = -1;
}

/*********************************************************************/

static void print_nav_name(FILE *out,struct btm const *date,unit__e unit,char sep)
{
  assert(out  != NULL);
  assert(date != NULL);
  assert(sep  >= ' ');
  assert(sep  <= '~');
  
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

static void print_nav_url(FILE *out,struct btm const *date,unit__e unit,char const *baseurl)
{
  assert(out  != NULL);
  assert(date != NULL);
  
  fprintf(out,"%s",baseurl);
  print_nav_name(out,date,unit,'/');
}

/*******************************************************************/

static void print_nav_title(FILE *out,Blog *blog,struct btm const *date,unit__e unit)
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
         entry = BlogEntryRead(blog,date);
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

static void handle_aflinks(HtmlToken token,char const *attrib,Blog *blog)
{
  assert(token  != NULL);
  assert(attrib != NULL);
  
  struct pair *src = HtmlParseGetPair(token,attrib);
  if (src != NULL)
  {
    for (size_t i = 0 ; i < blog->config.affiliatenum ; i++)
    {
      if (strncmp(src->value,blog->config.affiliates[i].proto,blog->config.affiliates[i].psize) == 0)
      {
        char buffer[BUFSIZ];
        struct pair *np;
        
        snprintf(
                buffer,
                sizeof(buffer),
                blog->config.affiliates[i].format,
                &src->value[blog->config.affiliates[i].psize + 1]
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

static void fixup_uri(BlogEntry *entry,HtmlToken token,char const *attrib,Blog *blog,Request const *request)
{
  struct pair *src;
  struct pair *np;
  
  assert(entry  != NULL);
  assert(entry->valid);
  assert(token  != NULL);
  assert(attrib != NULL);
  
  handle_aflinks(token,attrib,blog);
  
  src = HtmlParseGetPair(token,attrib);
  if ((src != NULL) && !uri_scheme(src->value))
  {
    char        buffer[BUFSIZ];
    char const *baseurl;
    
    /*---------------------------------------------------------------
    ; If pointing to an anchor point, no further processing required
    ;----------------------------------------------------------------*/
    
    if (src->value[0] == '#')
      return;
      
    /*-----------------------------------------------------
    ; Which URL to use?  Full or partial?
    ;-----------------------------------------------------*/
    
    if (request->f.fullurl)
      baseurl = blog->config.url;
    else
      baseurl = blog->config.baseurl;
      
    /*-----------------------------------------------------------------------
    ; all this to reassign the value, without ``knowing'' how the pair stuff
    ; actually works.  Perhaps add a PairReValue() call or something?
    ;
    ; Anyway, check to see if baseurl ends in a '/', and if not, we then add
    ; one, otherwise, don't.
    ;
    ; If baseurl ends in a '/', and src->value does not start with a '/', we
    ; end up with a double '//'.  We need to make sure we canonicalize all
    ; partial urls to either end in a '/', or not end in a '/'.
    ;------------------------------------------------------------------------*/
    
    if (src->value[0] == '/')
      snprintf(buffer,sizeof(buffer),"%s%s",baseurl,&src->value[1]);
    else if (isdigit(src->value[0]))
      snprintf(buffer,sizeof(buffer),"%s%s",baseurl,src->value);
    else
      snprintf(
        buffer,
        sizeof(buffer),
        "%s%04d/%02d/%02d/%s",
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

/***************************************************************
* TEMPLATE CALL BACK FUNCTIONS
****************************************************************/

static void cb_ad(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(data != NULL);
  assert(cbd->entry->valid);
  
  snprintf(
        fname,
        sizeof(fname),
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
  
  /*-------------------------------------
  ; we might also do a generic_cb() here
  ;--------------------------------------*/
  
  fcopy(out,cbd->ad);
}

/*********************************************************************/

static void cb_atom_categories(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  String  *cats;
  size_t   num;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  cats = tag_split(&num,cbd->entry->class);
  
  for (size_t i = 0 ; i < num ; i++)
  {
    cbd->adcat = fromstring(cats[i]);
    generic_cb("categories",out,cbd);
    free(cbd->adcat);
    cbd->adcat = NULL;
  }
  
  free(cats);
}

/************************************************************************/

static void cb_atom_category(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  FILE                 *eout;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  fputs(cbd->adcat,eout);
  fclose(eout);
}

/****************************************************************/

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

static void cb_begin_year(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fprintf(out,"%04d",cbd->blog->first.year);
}

/*******************************************************************/

static void cb_blog_adtag(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                 *tag;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->adtag == NULL)
    tag = UrlEncodeString(cbd->blog->config.adtag);
  else
    tag = UrlEncodeString(cbd->adtag);
  fputs(tag,out);
  free(tag);
}

/*********************************************************************/

static void cb_blog_adtag_entity(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  FILE                 *entityout;
  char const           *tag;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->adtag == NULL)
    tag = cbd->blog->config.adtag;
  else
    tag = cbd->adtag;
    
  entityout = fentity_encode_onwrite(out);
  if (entityout == NULL) return;
  fputs(tag,entityout);
  fclose(entityout);
}

/********************************************************************/

static void cb_blog_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.author.name,out);
}

/********************************************************************/

static void cb_blog_author_email(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.author.email,out);
}

/**********************************************************************/

static void cb_blog_class(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.class,out);
}

/**********************************************************************/

static void cb_blog_description(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.description,out);
}

/*********************************************************************/

static void cb_blog_name(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.name,out);
}

/*********************************************************************/

static void cb_blog_script(FILE *out,void *data)
{
  char *script;
  
  assert(out != NULL);
  (void)data;
  
  script = getenv("SCRIPT_NAME");
  if (script)
    fputs(script,out);
}

/**********************************************************************/

static void cb_blog_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->navunit == UNIT_PART)
  {
    BlogEntry *entry = (BlogEntry *)ListGetHead(&cbd->list);
    if (NodeValid(&entry->node) && entry->valid)
      fprintf(out,"%s - ",entry->title);
  }
  
  fputs(cbd->blog->config.name,out);
}

/*********************************************************************/

static void cb_blog_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY,cbd->blog->config.baseurl);
}

/*************************************************************************/

static void cb_blog_url_base(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.baseurl,out);
}

/*********************************************************************/

static void cb_blog_url_home(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->blog->config.url,out);
}

/**********************************************************************/

static void cb_comments(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->navunit != UNIT_PART)
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
  
  snprintf(
        fname,
        sizeof(fname),
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

static void cb_comments_check(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  fname[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  snprintf(
        fname,
        sizeof(fname),
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

static void cb_cond_hr(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation && (cbd->navunit == UNIT_PART))
    return;
    
  entry = cbd->entry;
  assert(entry->valid);
  if (btm_cmp_date(&entry->when,&cbd->last) == 0)
  {
    if (entry->when.part != cbd->last.part)
      generic_cb("cond.hr",out,data);
  }
}

/*********************************************************************/

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

static void cb_date_day_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_DAY,cbd->blog->config.baseurl);
}

/********************************************************************/

static void cb_edit(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.edit == false) return;
  generic_cb("edit",out,data);
}

/*********************************************************************/

static void cb_edit_adtag(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->adtag != NULL)
    fputs(cbd->request->adtag,out);
}

/*********************************************************************/

static void cb_edit_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->origauthor != NULL)
    fputs(cbd->request->origauthor,out);
  else
  {
    char *name = get_remote_user();
    fputs(name,out);
    free(name);
  }
}

/********************************************************************/

static void cb_edit_body(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->origbody)
    fputs(cbd->request->origbody,out);
}

/********************************************************************/

static void cb_edit_class(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->class)
    fputs(cbd->request->class,out);
}

/********************************************************************/

static void cb_edit_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->date != NULL)
    fputs(cbd->request->date,out);
  else
  {
    char       buffer[BUFSIZ];
    
    time_t     now = time(NULL);
    struct tm *ptm = localtime(&now);
    strftime(buffer,sizeof(buffer),"%Y/%m/%d",ptm);
    fputs(buffer,out);
  }
}

/********************************************************************/

static void cb_edit_status(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->status != NULL)
    fputs(cbd->request->status,out);
}

/**********************************************************************/

static void cb_edit_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->title != NULL)
    fputs(cbd->request->title,out);
}

/********************************************************************/

static void cb_entry(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.htmldump)
  {
    fcopy(out,stdin);
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

static void cb_entry_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->author,out);
}

/*************************************************************************/

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
        fixup_uri(entry,token,"HREF",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"BLOCKQUOTE") == 0)
        fixup_uri(entry,token,"CITE",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"IMG") == 0)
      {
        fixup_uri(entry,token,"SRC",cbd->blog,cbd->request);
        fixup_uri(entry,token,"LONGDESC",cbd->blog,cbd->request);
        fixup_uri(entry,token,"USEMAP",cbd->blog,cbd->request);
      }
      else if (strcmp(HtmlParseValue(token),"Q") == 0)
        fixup_uri(entry,token,"CITE",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"INS") == 0)
        fixup_uri(entry,token,"CITE",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"DEL") == 0)
        fixup_uri(entry,token,"CITE",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"FORM") == 0)
        fixup_uri(entry,token,"ACTION",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"INPUT") == 0)
      {
        fixup_uri(entry,token,"SRC",cbd->blog,cbd->request);
        fixup_uri(entry,token,"USEMAP",cbd->blog,cbd->request);
      }
      else if (strcmp(HtmlParseValue(token),"AREA") == 0)
        fixup_uri(entry,token,"HREF",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"OBJECT") == 0)
      {
        fixup_uri(entry,token,"CLASSID",cbd->blog,cbd->request);
        fixup_uri(entry,token,"CODEBASE",cbd->blog,cbd->request);
        fixup_uri(entry,token,"DATA",cbd->blog,cbd->request);
        fixup_uri(entry,token,"ARCHIVE",cbd->blog,cbd->request);
        fixup_uri(entry,token,"USEMAP",cbd->blog,cbd->request);
      }
      
      /*-----------------------------------------------------
      ; elements that can only appear in the <HEAD> section
      ; so commented out for now but here for reference
      ;------------------------------------------------------*/
      
#if 0
      else if (strcmp(HtmlParseValue(token),"LINK") == 0)
        fixup_uri(entry,token,"HREF",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"BASE") == 0)
        fixup_uri(entry,token,"HREF",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"HEAD") == 0)
        fixup_uri(entry,token,"PROFILE",cbd->blog,cbd->request);
      else if (strcmp(HtmlParseValue(token),"SCRIPT") == 0)
      {
        fixup_uri(entry,token,"SRC",cbd->blog,cbd->request);
        fixup_uri(entry,token,"FOR",cbd->blog,cbd->request);
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
  assert(out  != NULL);
  assert(data != NULL);
  
  FILE *eout = fentity_encode_onwrite(out);
  if (eout == NULL) return;
  cb_entry_body(eout,data);
  fclose(eout);
}

/*********************************************************************/

static void cb_entry_body_jsonified(FILE *out,void *data)
{
  assert(out  != NULL);
  assert(data != NULL);
  
  FILE *jout = fjson_encode_onwrite(out);
  if (jout == NULL) return;
  cb_entry_body(jout,data);
  fclose(jout);
}

/**********************************************************************/

static void cb_entry_class(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  char const *msg = cbd->blog->config.class;
  
  if (cbd->entry != NULL)
    msg = cbd->entry->class;
  else
  {
    if (cbd->navunit == UNIT_PART)
    {
      BlogEntry *entry = (BlogEntry *)ListGetHead(&cbd->list);
      if (NodeValid(&entry->node) && entry->valid)
        if (!empty_string(entry->class))
          msg = entry->class;
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

static void cb_entry_cond_author(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  
  if (strcmp(cbd->blog->config.author.name,cbd->entry->author) != 0)
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

static void cb_entry_date(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->entry->when,UNIT_DAY,cbd->blog->config.baseurl);
}

/*********************************************************************/

static void cb_entry_description(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  char const *msg = cbd->blog->config.description;
  
  if (cbd->navunit == UNIT_PART)
  {
    BlogEntry *entry = (BlogEntry *)ListGetHead(&cbd->list);
    
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

static void cb_entry_status(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fputs(cbd->entry->status,out);
}

/************************************************************************/

static void cb_entry_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  assert(cbd->entry->valid);
  fputs(cbd->entry->title,out);
}

/***********************************************************************/

static void cb_entry_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  BlogEntry            *entry;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  entry = cbd->entry;
  assert(entry->valid);
  print_nav_url(out,&entry->when,UNIT_PART,cbd->blog->config.baseurl);
}

/**********************************************************************/

static void cb_generator(FILE *out,void *data)
{
  assert(out != NULL);
  (void)data;
  
  fprintf(
    out,
    "mod_blog %s, %s, %s, %s"
    "",
    PROG_VERSION,
    cgilib_version,
    LUA_RELEASE,
    gdbm_version
  );
}

/*********************************************************************/

static void cb_json_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char const           *sep = "";
  
  assert(out  != NULL);
  assert(data != NULL);
  
  for
  (
    BlogEntry *entry = (BlogEntry *)ListRemHead(&cbd->list);
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

static void cb_navigation_bar(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false) return;
  generic_cb("navigation.bar",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_next(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navnext == false) return;
  generic_cb("navigation.bar.next",out,data);
}

/*******************************************************************/

static void cb_navigation_bar_prev(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navprev == false) return;
  generic_cb("navigation.bar.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_current(FILE *out,void *data)
{
  assert(out != NULL);
  generic_cb("navigation.current",out,data);
}

/********************************************************************/

static void cb_navigation_current_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->blog->last,UNIT_MONTH,cbd->blog->config.baseurl);
}

/*********************************************************************/

static void cb_navigation_first_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false)
    print_nav_title(out,cbd->blog,&cbd->blog->first,UNIT_PART);
  else
    print_nav_title(out,cbd->blog,&cbd->blog->first,cbd->navunit);
}

/*******************************************************************/

static void cb_navigation_first_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false)
    print_nav_url(out,&cbd->blog->first,UNIT_PART,cbd->blog->config.baseurl);
  else
    print_nav_url(out,&cbd->blog->first,cbd->navunit,cbd->blog->config.baseurl);
}

/********************************************************************/

static void cb_navigation_last_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false)
    print_nav_title(out,cbd->blog,&cbd->blog->last,UNIT_PART);
  else
    print_nav_title(out,cbd->blog,&cbd->blog->last,cbd->navunit);
}

/*****************************************************************/

static void cb_navigation_last_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false)
    print_nav_url(out,&cbd->blog->last,UNIT_PART,cbd->blog->config.baseurl);
  else
    print_nav_url(out,&cbd->blog->last,cbd->navunit,cbd->blog->config.baseurl);
}

/******************************************************************/

static void cb_navigation_link(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navigation == false) return;
  generic_cb("navigation.link",out,data);
}

/********************************************************************/

static void cb_navigation_link_next(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navnext == false) return;
  generic_cb("navigation.link.next",out,data);
}

/*******************************************************************/

static void cb_navigation_link_prev(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->request->f.navprev == false) return;
  generic_cb("navigation.link.prev",out,data);
}

/*******************************************************************/

static void cb_navigation_next_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_title(out,cbd->blog,&cbd->next,cbd->navunit);
}

/********************************************************************/

static void cb_navigation_next_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->next,cbd->navunit,cbd->blog->config.baseurl);
}

/*******************************************************************/

static void cb_navigation_prev_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_title(out,cbd->blog,&cbd->previous,cbd->navunit);
}

/*******************************************************************/

static void cb_navigation_prev_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  print_nav_url(out,&cbd->previous,cbd->navunit,cbd->blog->config.baseurl);
}

/********************************************************************/

static void cb_now_year(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fprintf(out,"%04d",cbd->blog->now.year);
}

/*******************************************************************/

static void cb_request_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                 *tum;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  tum = tumbler_canonical(&cbd->request->tumbler);
  fprintf(out,"%s%s",cbd->blog->config.url,tum);
  free(tum);
}

/***********************************************************************/

static void cb_robots_index(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (cbd->navunit == UNIT_PART)
    fputs("index",out);
  else
    fputs("noindex",out);
}

/********************************************************************/

static void cb_rss_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  for
  (
    BlogEntry *entry = (BlogEntry *)ListRemHead(&cbd->list);
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
  fprintf(out,"%s",cbd->blog->config.url);
  print_nav_name(out,&cbd->entry->when,UNIT_PART,'/');
}

/********************************************************************/

static void cb_rss_pubdate(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  struct tm            *ptm;
  char                 buffer[BUFSIZ];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  ptm = gmtime(&cbd->blog->tnow);
  strftime(buffer,sizeof(buffer),"%a, %d %b %Y %H:%M:%S GMT",ptm);
  fputs(buffer,out);
}

/********************************************************************/

static void cb_rss_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fprintf(out,"%s",cbd->blog->config.url);
}

/*******************************************************************/

static void cb_update_time(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  tmpbuf[24];
  
  assert(out  != NULL);
  assert(data != NULL);
  
  strftime(tmpbuf,sizeof(tmpbuf),"%FT%TZ",gmtime(&cbd->blog->tnow));
  fputs(tmpbuf,out);
}

/*******************************************************************/

static void cb_webmention(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  char                  fname[FILENAME_MAX];
  int                   rc;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  snprintf(
         fname,
         sizeof(fname),
         "%04d/%02d/%02d/%d.webmention",
         cbd->entry->when.year,
         cbd->entry->when.month,
         cbd->entry->when.day,
         cbd->entry->when.part
  );
  
  cbd->wm = fopen(fname,"r");
  if (cbd->wm == NULL) return;
  rc = fcntl(
              fileno(cbd->wm),
              F_SETLKW,
              &(struct flock){
                .l_type   = F_RDLCK,
                .l_start  = 0,
                .l_whence = SEEK_SET,
                .l_len    = 0,
              }
            );
  if (rc < 0)
  {
    syslog(LOG_DEBUG,"fileno('%s') = %d",fname,fileno(cbd->wm));
    syslog(LOG_ERR,"fcntl('%s',F_SETLKW) = %s",fname,strerror(errno));
    fclose(cbd->wm);
    return;
  }
  
  generic_cb("webmention",out,data);
  
  rc = fcntl(
              fileno(cbd->wm),
              F_SETLKW,
              &(struct flock){
                .l_type   = F_UNLCK,
                .l_start  = 0,
                .l_whence = SEEK_SET,
                .l_len    = 0,
              }
            );
  if (rc < 0)
    syslog(LOG_ERR,"fcntl('%s',F_UNLCK) = %s",fname,strerror(errno));
  fclose(cbd->wm);
  cbd->wm = NULL;
}

/*******************************************************************/

static void cb_webmention_item(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  size_t                 buflen = 0;
  ssize_t                len;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  while((len = getline(&cbd->wmurl,&buflen,cbd->wm)) != -1)
  {
    char *p = strchr(cbd->wmurl,'\t');
    if (p == NULL)
      cbd->wmtitle = cbd->wmurl;
    else
    {
      *p++ = '\0';
      cbd->wmtitle = p;
    }
    cbd->wmurl[len - 1] = '\0'; /* remove trailing '\n' */
    generic_cb("webmention.item",out,data);
    free(cbd->wmurl);
    cbd->wmurl   = NULL;
    cbd->wmtitle = NULL;
    buflen       = 0;
  }
  
  free(cbd->wmurl);
}

/*******************************************************************/
static void cb_webmention_title(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  fputs(cbd->wmtitle,out);
}

/*******************************************************************/

static void cb_webmention_url(FILE *out,void *data)
{
  struct callback_data *cbd = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  fputs(cbd->wmurl,out);
}

/*******************************************************************/

static void cb_xyzzy(FILE *out,void *data)
{
  assert(out != NULL);
  (void)data;
  fputs("Nothing happens.",out);
}

/********************************************************************/

void generic_cb(char const *which,FILE *out,void *data)
{
  /*-----------------------------------------------------
  ; the following table needs to be in alphabetical order
  ;------------------------------------------------------*/
  
  static struct chunk_callback const callbacks[] =
  {
    { "ad"                     , cb_ad                     } , /* template "ad" */
    { "ad.content"             , cb_ad_content             } ,
    { "atom.categories"        , cb_atom_categories        } , /* template "categories" */
    { "atom.category"          , cb_atom_category          } ,
    { "atom.entry"             , cb_atom_entry             } , /* template "entry" */
    { "begin.year"             , cb_begin_year             } ,
    { "blog.adtag"             , cb_blog_adtag             } ,
    { "blog.adtag.entity"      , cb_blog_adtag_entity      } ,
    { "blog.author"            , cb_blog_author            } ,
    { "blog.author.email"      , cb_blog_author_email      } ,
    { "blog.class"             , cb_blog_class             } ,
    { "blog.description"       , cb_blog_description       } ,
    { "blog.name"              , cb_blog_name              } ,
    { "blog.script"            , cb_blog_script            } ,
    { "blog.title"             , cb_blog_title             } ,
    { "blog.url"               , cb_blog_url               } ,
    { "blog.url.base"          , cb_blog_url_base          } ,
    { "blog.url.home"          , cb_blog_url_home          } ,
    { "comments"               , cb_comments               } , /* template "comments" */
    { "comments.body"          , cb_comments_body          } ,
    { "comments.check"         , cb_comments_check         } ,
    { "comments.filename"      , cb_comments_filename      } ,
    { "cond.hr"                , cb_cond_hr                } , /* template "cond.hr" */
    { "date.day"               , cb_date_day               } ,
    { "date.day.normal"        , cb_date_day_normal        } ,
    { "date.day.url"           , cb_date_day_url           } ,
    { "edit"                   , cb_edit                   } , /* template "edit" */
    { "edit.adtag"             , cb_edit_adtag             } ,
    { "edit.author"            , cb_edit_author            } ,
    { "edit.body"              , cb_edit_body              } ,
    { "edit.class"             , cb_edit_class             } ,
    { "edit.date"              , cb_edit_date              } ,
    { "edit.status"            , cb_edit_status            } ,
    { "edit.title"             , cb_edit_title             } ,
    { "entry"                  , cb_entry                  } , /* template "entry" */
    { "entry.author"           , cb_entry_author           } ,
    { "entry.body"             , cb_entry_body             } ,
    { "entry.body.entified"    , cb_entry_body_entified    } ,
    { "entry.body.jsonified"   , cb_entry_body_jsonified   } ,
    { "entry.class"            , cb_entry_class            } ,
    { "entry.class.jsonified"  , cb_entry_class_jsonified  } ,
    { "entry.cond.author"      , cb_entry_cond_author      } , /* template "entry.cond.author" */
    { "entry.cond.date"        , cb_entry_cond_date        } , /* template "entry.cond.date" */
    { "entry.date"             , cb_entry_date             } ,
    { "entry.description"      , cb_entry_description      } ,
    { "entry.id"               , cb_entry_id               } ,
    { "entry.linkdate"         , cb_entry_linkdate         } ,
    { "entry.linkdated"        , cb_entry_linkdated        } ,
    { "entry.name"             , cb_entry_name             } ,
    { "entry.path"             , cb_entry_path             } ,
    { "entry.pubdate"          , cb_entry_pubdate          } ,
    { "entry.pubdatetime"      , cb_entry_pubdatetime      } ,
    { "entry.status"           , cb_entry_status           } ,
    { "entry.title"            , cb_entry_title            } ,
    { "entry.url"              , cb_entry_url              } ,
    { "generator"              , cb_generator              } ,
    { "json.item"              , cb_json_item              } , /* template "item" */
    { "navigation.bar"         , cb_navigation_bar         } , /* template "navigation.bar" */
    { "navigation.bar.next"    , cb_navigation_bar_next    } , /* template "navigation.bar.next" */
    { "navigation.bar.prev"    , cb_navigation_bar_prev    } , /* template "navigation.bar.prev" */
    { "navigation.current"     , cb_navigation_current     } , /* template "navigation.current" */
    { "navigation.current.url" , cb_navigation_current_url } ,
    { "navigation.first.title" , cb_navigation_first_title } ,
    { "navigation.first.url"   , cb_navigation_first_url   } ,
    { "navigation.last.title"  , cb_navigation_last_title  } ,
    { "navigation.last.url"    , cb_navigation_last_url    } ,
    { "navigation.link"        , cb_navigation_link        } , /* template "navigation.link" */
    { "navigation.link.next"   , cb_navigation_link_next   } , /* template "navigation.link.next" */
    { "navigation.link.prev"   , cb_navigation_link_prev   } , /* template "navigation.link.prev" */
    { "navigation.next.title"  , cb_navigation_next_title  } ,
    { "navigation.next.url"    , cb_navigation_next_url    } ,
    { "navigation.prev.title"  , cb_navigation_prev_title  } ,
    { "navigation.prev.url"    , cb_navigation_prev_url    } ,
    { "now.year"               , cb_now_year               } ,
    { "request.url"            , cb_request_url            } ,
    { "robots.index"           , cb_robots_index           } ,
    { "rss.item"               , cb_rss_item               } , /* template "item" */
    { "rss.item.url"           , cb_rss_item_url           } ,
    { "rss.pubdate"            , cb_rss_pubdate            } ,
    { "rss.url"                , cb_rss_url                } ,
    { "update.time"            , cb_update_time            } ,
    { "webmention"             , cb_webmention             } , /* template "webmention" */
    { "webmention.item"        , cb_webmention_item        } , /* template "webmention.item" */
    { "webmention.title"       , cb_webmention_title       } ,
    { "webmention.url"         , cb_webmention_url         } ,
    { "xyzzy"                  , cb_xyzzy                  } ,
  };
  
  assert(which != NULL);
  assert(out   != NULL);
  assert(data  != NULL);
  
  struct callback_data *cbd = data;
  
  Chunk templates = ChunkNew(cbd->template->template,callbacks,sizeof(callbacks) / sizeof(callbacks[0]));
  ChunkProcess(templates,which,out,data);
  ChunkFree(templates);
  fflush(out);
}

/*********************************************************************/

void generic_main(FILE *out,struct callback_data *cbd)
{
  char buf[64];
  
  assert(out != NULL);
  assert(cbd != NULL);
  
  if (cbd->request->f.cgi)
  {
    if ((cbd->status == HTTP_OKAY) && HttpNotModified(cbd->blog->lastmod))
    {
      fprintf(
        out,
        "Status: %d\r\n"
        "Content-Length: 0\r\n"
        "Last-Modified: %s\r\n"
        "\r\n",
        HTTP_NOTMODIFIED,
        HttpTimeStamp(buf,64,cbd->blog->lastmod)
      );
      return;
    }
    
    fprintf(
        out,
        "Status: %d\r\n"
        "Content-Type: text/html\r\n"
        "Last-Modified: %s\r\n"
        "\r\n",
        cbd->status,
        HttpTimeStamp(buf,64,cbd->blog->lastmod)
    );
  }
  generic_cb("main",out,cbd);
}

/*********************************************************************/
