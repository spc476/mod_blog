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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include <cgilib6/htmltok.h>
#include <cgilib6/util.h>
#include <cgilib6/cgi.h>
#include <cgilib6/chunk.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "wbtum.h"
#include "frontend.h"
#include "fix.h"
#include "blogutil.h"
#include "backend.h"
#include "globals.h"

/*****************************************************************/

static void	   calculate_previous		(struct btm);
static void	   calculate_next		(struct btm);
static const char *mime_type			(char *);
static int	   display_file			(FILE *,Tumbler);
static char       *tag_collect			(List *);
static char	  *tag_pick                     (const char *);
static void	   free_entries			(List *);

/************************************************************************/

int generate_pages(Request req __attribute__((unused)))
{
  for (size_t i = 0 ; i < c_numtemplates; i++)
  {
    FILE *out;
    
    out = fopen(c_templates[i].file,"w");
    if (out == NULL)
    {
      syslog(LOG_ERR,"%s: %s",c_templates[i].file,strerror(errno));
      continue;
    }
    
    (*c_templates[i].pagegen)(&c_templates[i],out,g_blog);
    fclose(out);
  }
  
  return 0;
}

/******************************************************************/

int pagegen_items(
	const template__t *const restrict template,
	FILE              *const restrict out,
	Blog               const restrict blog
)
{
  struct btm            thisday;
  char                 *tags;
  struct callback_data  cbd;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(blog     != NULL);
  
  g_templates  = template->template;
  gd.f.fullurl = template->fullurl;
  gd.f.reverse = template->reverse;
  thisday      = blog->now;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  ListInit(&cbd.list);
  if (template->reverse)
    BlogEntryReadXD(g_blog,&cbd.list,&thisday,template->items);
  else
    BlogEntryReadXU(g_blog,&cbd.list,&thisday,template->items);
  
  tags      = tag_collect(&cbd.list);
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int pagegen_days(
	const template__t *const restrict template,
	FILE              *const restrict out,
	Blog               const restrict blog
)
{
  struct btm            thisday;
  size_t                days;
  char                 *tags;
  struct callback_data  cbd;
  bool                  added;
  
  assert(template != NULL);
  assert(out      != NULL);
  assert(blog     != NULL);

  g_templates  = template->template;
  gd.f.fullurl = false;
  gd.f.reverse = true;
  thisday      = blog->now;
  
  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  
  for (days = 0 , added = false ; days < template->items ; )
  {
    BlogEntry entry;
    
    if (btm_cmp(&thisday,&g_blog->first) < 0) break;
    
    entry = BlogEntryRead(g_blog,&thisday);
    if (entry)
    {
      assert(entry->valid);
      ListAddTail(&cbd.list,&entry->node);
      added = true;
    }
    
    thisday.part--;
    if (thisday.part == 0)
    {
      thisday.part = 23;
      btm_sub_day(&thisday);
      if (added)
        days++;
      added = false;
    }
  }
  
  tags      = tag_collect(&cbd.list);
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return 0;
}

/************************************************************************/

int tumbler_page(FILE *out,Tumbler spec)
{
  struct btm            start;
  struct btm            end;
  TumblerUnit           tu1;
  TumblerUnit           tu2;
  int                   nu1;
  int                   nu2;
  char                 *tags;
  struct callback_data  cbd;
  
  assert(out  != NULL);
  assert(spec != NULL);
  
  gd.f.fullurl = false;
  
  /*---------------------------------------------------------
  ; easy checks first.  Get these out of the way ... 
  ;---------------------------------------------------------*/
  
  if (spec->flags.error)
    return(HTTP_NOTFOUND);
  
  if (spec->flags.redirect)
    return(HTTP_NOTIMP);
  
  if (spec->flags.file)
  {
    display_file(out,spec);
    return 0;	/* XXX hack for now */
  }

  memset(&cbd,0,sizeof(struct callback_data));
  ListInit(&cbd.list);
  
  tu1 = (TumblerUnit)ListGetHead(&spec->units);
  tu2 = (TumblerUnit)NodeNext(&tu1->node);
  
  /*-----------------------------------------------------
  ; validate input tumbler from user.  
  ;----------------------------------------------------*/

  if (
       (tu1->entry[YEAR]) 
       && (tu1->entry[YEAR] < (unsigned)g_blog->first.year)
     ) 
    return(1);
  if (tu1->entry[MONTH]  > 12) return(1);
  if ((tu1->entry[MONTH] == 0) && (tu1->entry[DAY])) return(1);
  if ((tu1->entry[MONTH]) && (tu1->entry[DAY] > max_monthday(tu1->entry[YEAR],tu1->entry[MONTH])))
    return(1);

  if (tu1->type == TUMBLER_RANGE)
  {
    if (
         (tu2->entry[YEAR] != 0)
         && (tu2->entry[YEAR] < (unsigned)g_blog->first.year)
       )
      return(1);
    if (tu2->entry[MONTH]  > 12) return(1);
    if ((tu2->entry[MONTH] == 0) && (tu2->entry[DAY])) return(1);
    if ((tu2->entry[MONTH]) && (tu2->entry[DAY]   > max_monthday(tu2->entry[YEAR],tu2->entry[MONTH])))
      return(1);
  }

  /*------------------------------------------------------------------------
  ; I junked the use of struct tm as it was just too error prone.  I defined
  ; a struct btm with only the fields I need, and with the definitions I
  ; required.
  ;
  ; The tumbler code will set unspecified fields to 0.  In this case, we
  ; want to start with minimum legal values.
  ;-----------------------------------------------------------------------*/

  nu1         = PART;
  start.part  = (tu1->entry[PART]  == 0) ? nu1 = DAY     , 1                  : tu1->entry[PART];
  start.day   = (tu1->entry[DAY]   == 0) ? nu1 = MONTH   , 1                  : tu1->entry[DAY];
  start.month = (tu1->entry[MONTH] == 0) ? nu1 = YEAR    , 1                  : tu1->entry[MONTH];
  start.year  = (tu1->entry[YEAR]  == 0) ? nu1 = YEAR    , g_blog->first.year : (int)tu1->entry[YEAR];
  
  if (start.day > max_monthday(start.year,start.month))
    return(1);				/* invalid day */

  /*------------------------------------------------------------------------
  ; Make some things easier if for a single request, we set a range and let
  ; the result fall out of normal processing.
  ;-----------------------------------------------------------------------*/
  
  if (tu1->type == TUMBLER_SINGLE)
  {
    gd.f.navigation = true;
    nu2        = PART;
    end.year   = (tu1->entry[YEAR]  == 0) ? nu2 = YEAR  , g_blog->now.year                 : (int)tu1->entry[YEAR];
    end.month  = (tu1->entry[MONTH] == 0) ? nu2 = MONTH , 12                               : tu1->entry[MONTH];
    end.day    = (tu1->entry[DAY]   == 0) ? nu2 = DAY   , max_monthday(end.year,end.month) : tu1->entry[DAY];
    end.part   = (tu1->entry[PART]  == 0) ? nu2 = PART  , 23                               : tu1->entry[PART];
    
    /*---------------------------------------------------------------------
    ; XXX---scan-build revealed that nu2 is not used, so it could be
    ; removed.  But, in looking over the code, I think we want to set
    ; gd.navunit to nu2 (which would make nu1 redundant).  I have to think
    ; about this.
    ;----------------------------------------------------------------------*/
    
    gd.navunit = nu1;
    
    calculate_previous(start);
    calculate_next(end);
  }
  else
  {
    /*----------------------------------------------------------------------
    ; fix the ending date.  In this case, the unspecified fields will be set
    ; to the their maximum legal value.
    ;---------------------------------------------------------------------*/
    
    end.year  = (tu2->entry[YEAR]  == 0) ? g_blog->now.year                 : (int)tu2->entry[YEAR];
    end.month = (tu2->entry[MONTH] == 0) ? 12                               : tu2->entry[MONTH];
    end.day   = (tu2->entry[DAY]   == 0) ? max_monthday(end.year,end.month) : tu2->entry[DAY];
    end.part  = (tu2->entry[PART]  == 0) ? 23                               : tu2->entry[PART];
    
    /*------------------------------------------------------------------------
    ; swap tumblers if required.  If the parts are the default values, swap
    ; them, as they probably weren't specified and need swapping as well.
    ;
    ; Example:
    ;
    ;     2000/02/04    - 01/26 
    ;  -> 2000/02/04.1  - 2000/01/26.23
    ;  -> 2000/01/26.23 - 2000/02/04.1
    ;  -> 2000/01/26.1  - 2000/02/04.23
    ;
    ;-----------------------------------------------------------------------*/
    
    if (btm_cmp(&start,&end) > 0)
    {
      struct btm tmp;
      
      gd.f.reverse = true;
      tmp   = start;
      start = end;
      end   = tmp;
      
      if ((start.part == 23) && (end.part == 1))
      {
        start.part = 1;
        end.part = 23;
      }
    }
  }

  if (end.day > max_monthday(end.year,end.month))
    return(1);

  /*---------------------------------------------------------------------
  ; Okay, sanity checking here ... These should be true once we hit this
  ; spot of the code.
  ;---------------------------------------------------------------------*/
  
  assert(start.year  >= 1);
  assert(start.month >= 1);
  assert(start.month <= 12);
  assert(start.day   >= 1);
  assert(start.day   <= max_monthday(start.year,start.month));
  
  assert(end.year  >= 1);
  assert(end.month >= 1);
  assert(end.month <= 12);
  assert(end.day   >= 1);
  assert(end.day   <= max_monthday(end.year,end.month));
  
  /*-------------------------------------------------------------------------
  ; okay, resume processing ... bound against the starting time of the blog,
  ; and the current time.
  ;------------------------------------------------------------------------*/
  
  if (btm_cmp(&start,&g_blog->first) < 0)
    start = g_blog->first;
  if (btm_cmp(&end,&g_blog->now) > 0)
    end = g_blog->now;
  
  /*-----------------------------------------------------------------------
  ; From here on out, it's pretty straight forward.  read a day, if it has
  ; entries, add it to the list, otherwise, continue on, advancing (or
  ; retreading) the days as we go ...
  ;----------------------------------------------------------------------*/

  /*-----------------------------------------------------------
  ; these four lines replaced 65 very confused lines of code.
  ;-----------------------------------------------------------*/
  
  if (gd.f.reverse)
    BlogEntryReadBetweenD(g_blog,&cbd.list,&end,&start);
  else
    BlogEntryReadBetweenU(g_blog,&cbd.list,&start,&end);
  
  tags      = tag_collect(&cbd.list);  
  cbd.adtag = tag_pick(tags);
  free(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  free(cbd.adtag);
  return(0);
}

/******************************************************************/

static void calculate_previous(struct btm start)
{
  gd.previous = start;

  switch(gd.navunit)
  {
    case YEAR:
         if (start.year == g_blog->first.year)
           gd.f.navprev = false;
         else
           gd.previous.year = start.year - 1;
         break;
    case MONTH:
         if (
              (start.year == g_blog->first.year) 
              && (start.month == g_blog->first.month)
            )
           gd.f.navprev = false;
         else
	   btm_sub_month(&gd.previous);
         break;
    case DAY:
         if (btm_cmp_date(&start,&g_blog->first) == 0)
           gd.f.navprev = false;
         else
         {
           btm_sub_day(&gd.previous);
           
           while(btm_cmp(&gd.previous,&g_blog->first) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
               btm_sub_day(&gd.previous);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
                      
           gd.f.navprev = false;
         }
         break;
    case PART:
         if (btm_cmp(&start,&g_blog->first) == 0)
           gd.f.navprev = false;
         else
         {
	   btm_sub_part(&gd.previous);

           while(btm_cmp(&gd.previous,&g_blog->first) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
	       btm_sub_part(&gd.previous);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           
           gd.f.navprev = false;
         }
         break;
    default:
         assert(0);
  }
}

/******************************************************************/

static void calculate_next(struct btm end)
{
  gd.next = end;
  
  switch(gd.navunit)
  {
    case YEAR:
         if (end.year == g_blog->now.year)
           gd.f.navnext = false;
         else
           gd.next.year  = end.year + 1;
         break;
    case MONTH:
         if (
              (end.year == g_blog->now.year) 
              && (end.month == g_blog->now.month)
            )
           gd.f.navnext = false;
         else
           btm_add_month(&gd.next);
         break;
    case DAY:
         if (btm_cmp_date(&end,&g_blog->now) == 0)
           gd.f.navnext = false;
         else
         {
           btm_add_day(&gd.next);
	   gd.next.part = 1;
           
           while(btm_cmp(&gd.next,&g_blog->now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               btm_add_day(&gd.next);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           
           gd.f.navnext = false;
         }
         break;
    case PART:
         if (btm_cmp(&end,&g_blog->now) == 0)
           gd.f.navnext = false;
	 else
	 {
	   gd.next.part++;
         
           while(btm_cmp(&gd.next,&g_blog->now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               gd.next.part = 1;
               btm_add_day(&gd.next);
               continue;
             }
             assert(entry->valid);
             BlogEntryFree(entry);
             return;
           }
           gd.f.navnext = false;
	 }
         break;
    default:
         assert(0);
  }
}

/******************************************************************/

static const char *mime_type(char *filename)
{
  int                         i;
  static const struct dstring types[] = 
  {
    { ".jpeg"		, "image/jpeg"	},
    { ".jpg"		, "image/jpeg"	},
    { ".gif"		, "image/gif"	},
    { ".png"		, "image/png"	},
    { ".x-html"		, "text/x-html" },
    { ".html"		, "text/html"   },
    { ".htm"            , "text/html"   },
    { ".txt"		, "text/plain"	},
    { ".css"		, "text/css"	},
    { ".mp4"		, "video/mp4"	},
    { ".mov"		, "video/quicktime" },
    { ".gz"		, "application/x-gzip" },
    { NULL		, NULL		}
  };
  
  assert(filename != NULL);
  
  for (i = 0 ; types[i].s1 != NULL ; i++)
  {
    if (strstr(filename,types[i].s1) != NULL)
      return(types[i].s2);
  }
  
  return("text/plain");
}

/******************************************************************/

static int display_file(FILE * out,Tumbler spec)
{
  TumblerUnit tu;
  char        fname[FILENAME_MAX];
  
  assert(out  != NULL);
  assert(spec != NULL);

  tu = (TumblerUnit)ListGetHead(&spec->units);
  
  sprintf(
           fname,
           "%4d/%02d/%02d/%s",
           tu->entry[YEAR],
           tu->entry[MONTH],
           tu->entry[DAY],
           tu->file
         );
         
  if (gd.cgi)
  {
    struct stat  status;
    FILE        *in;
    const char  *type;
    int          rc;
    
    rc = stat(fname,&status);
    if (rc == -1) 
    {
      if (errno == ENOENT)
        (*gd.req->error)(gd.req,HTTP_NOTFOUND,"%s: %s",fname,strerror(errno));
      else if (errno == EACCES)
        (*gd.req->error)(gd.req,HTTP_FORBIDDEN,"%s: %s",fname,strerror(errno));
      else
        (*gd.req->error)(gd.req,HTTP_ISERVERERR,"%s: %s",fname,strerror(errno));
      return(1);
    }

    in = fopen(fname,"r");
    if (in == NULL)
    {
      (*gd.req->error)(gd.req,HTTP_NOTFOUND,"%s: some internal error",fname);
      return(1);
    }
    
    type = mime_type(tu->file);

    if (strcmp(type,"text/x-html") == 0)
    {
      gd.htmldump = in;
      fputs("Status: 200\r\nContent-type: text/html\r\n\r\n",out);
      generic_cb("main",out,NULL);
    }
    else
    {
      fprintf(
      	out,
      	"Status: 200\r\n"
      	"Content-Type: %s\r\n"
      	"Content-Length: %lu\r\n"
      	"\r\n",
      	type,
      	(unsigned long)status.st_size
      );
      
      fcopy(out,in);
    }
    fclose(in);
  }
  else
    fprintf(out,"File to open: %s\n",fname);

  return(0);
}

/*****************************************************************/

static char *tag_collect(List *list)
{
  BlogEntry entry;
  
  /*-----------------------------------------------------------------------
  ; this function used to collect all the class tags from all the entries,
  ; but I think any advertising that's placed near the top of the page will
  ; do better if it's based off the first entry to be displayed.
  ;-----------------------------------------------------------------------*/
  
  assert(list != NULL);
  
  entry = (BlogEntry)ListGetHead(list);
  
  if (!NodeValid(&entry->node) || empty_string(entry->class))
    return strdup(c_adtag);
  else
    return strdup(entry->class);
}

/********************************************************************/

static char *tag_pick(const char *tag)
{
  String *pool;
  size_t  num;
  size_t  r;
  char   *pick;
  
  assert(tag != NULL);

  if (empty_string(tag))
    return(strdup(c_adtag));
  
  pool = tag_split(&num,tag);

  /*------------------------------------------------------------------------
  ; if num is 0, then the tag string was malformed (basically, started with
  ; a ',') and therefore, we fall back to the default adtag.
  ;-----------------------------------------------------------------------*/

  if (num)
  {
    r  = (((double)rand() / (double)RAND_MAX) * (double)num); 
    assert(r < num);
    pick = fromstring(pool[r]);
  }
  else
    pick = strdup(c_adtag);
  
  free(pool);
  return(pick);
}
 
/******************************************************************/

static void free_entries(List *list)
{
  BlogEntry entry;
  
  for
  (
    entry = (BlogEntry)ListRemHead(list);
    NodeValid(&entry->node);
    entry = (BlogEntry)ListRemHead(list)
  )
  {
    assert(entry->valid);
    BlogEntryFree(entry);
  }
}

/******************************************************************/

