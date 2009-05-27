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

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/htmltok.h>
#include <cgilib/util.h>
#include <cgilib/stream.h>
#include <cgilib/cgi.h>
#include <cgilib/chunk.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "wbtum.h"
#include "frontend.h"
#include "fix.h"
#include "globals.h"
#include "blogutil.h"

#define max(a,b)	((a) > (b)) ? (a) : (b)

/*****************************************************************/

static void	   calculate_previous		(struct btm);
static void	   calculate_next		(struct btm);
static const char *mime_type			(char *);
static int	   display_file			(Stream,Tumbler);
static int	   rss_page			(Stream,struct btm *,int,int);
static char       *tag_collect			(List *);
static char	  *tag_pick                     (const char *);
static void	   free_entries			(List *);

/************************************************************************/

int generate_pages(Request req)
{
  Stream out;
  int    rc = 0;

  ddt(req != NULL);

  /*------------------------------------------------------
  ; Why do we call set_time() here?  Because of a subtle bug.  Basically,
  ; the program starts, let's say at time T, and we add an entry.  By the
  ; time we're finished, it may be T+1s, and the content generation will
  ; consider the newly added entry as being in the future (for very small
  ; values of "future").  By doing this, we cause a temporal rift and slam
  ; the past up to the future (or slam the future into the
  ; here-and-now---whatever).
  ;
  ; Basically, we are now assured of actually generating the new content we
  ; just added.
  ;
  ; Just so you know.
  ;-------------------------------------------------------*/

  set_time();

  out = FileStreamWrite(c_daypage,FILE_RECREATE);  
  if (out == NULL) 
  {
    return(ERR_ERR);
  }
  rc = primary_page(out,gd.now.year,gd.now.month,gd.now.day,gd.now.part);
  StreamFree(out);
  
  if (c_rsstemplates)
  {
    g_templates = c_rsstemplates;
    out         = FileStreamWrite(c_rssfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = rss_page(out,&gd.now,TRUE,cf_rssreverse);
    StreamFree(out);
  }

  if (c_atomtemplates)
  {
    g_templates = c_atomtemplates;
    out         = FileStreamWrite(c_atomfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = rss_page(out,&gd.now,TRUE,cf_rssreverse);
    StreamFree(out);
  }
  
  if (c_tabtemplates)
  {
    g_templates = c_tabtemplates;
    out         = FileStreamWrite(c_tabfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = rss_page(out,&gd.now,FALSE,cf_tabreverse);
    StreamFree(out);
  }
  
  return(rc);
}

/******************************************************************/

int tumbler_page(Stream out,Tumbler spec)
{
  struct btm            start;
  struct btm            end;
  TumblerUnit           tu1;
  TumblerUnit           tu2;
  int                   nu1;
  int                   nu2;
  char                 *tags;
  struct callback_data  cbd;
  
  ddt(out  != NULL);
  ddt(spec != NULL);
  
  gd.f.fullurl = FALSE;
  
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
    return(ERR_OKAY);	/* XXX hack for now */
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
       && (tu1->entry[YEAR] < gd.begin.year)
     ) 
    return(1);
  if (tu1->entry[MONTH]  <  0) return(1);
  if (tu1->entry[MONTH]  > 12) return(1);
  if ((tu1->entry[MONTH] == 0) && (tu1->entry[DAY])) return(1);
  if (tu1->entry[DAY]    <  0) return(1);
  if ((tu1->entry[MONTH]) && (tu1->entry[DAY] > max_monthday(tu1->entry[YEAR],tu1->entry[MONTH])))
    return(1);

  if (tu1->type == TUMBLER_RANGE)
  {
    TumblerUnit tmp;
    
    if (
         (tu2->entry[YEAR] != 0)
         && (tu2->entry[YEAR] < gd.begin.year)
       )
      return(1);
    if (tu2->entry[MONTH]  <  0) return(1);
    if (tu2->entry[MONTH]  > 12) return(1);
    if ((tu2->entry[MONTH] == 0) && (tu2->entry[DAY])) return(1);
    if (tu2->entry[DAY]    <  0) return(1);
    if ((tu2->entry[MONTH]) && (tu2->entry[DAY]   > max_monthday(tu2->entry[YEAR],tu2->entry[MONTH])))
      return(1);
      
    /*--------------------------------------------------
    ; swap the tumblers if we can---it'll save us a crap
    ; of work later.
    ;--------------------------------------------------*/
    
    if (tu1->entry[YEAR] > tu2->entry[YEAR])
    {
      gd.f.reverse = TRUE;
      tmp = tu1;
      tu1 = tu2;
      tu2 = tmp;
      tu1->type = TUMBLER_RANGE;
      tu2->type = TUMBLER_SINGLE;
    }
    else if (tu1->entry[MONTH] > tu2->entry[MONTH])
    {
      gd.f.reverse = TRUE;
      tmp = tu1;
      tu1 = tu2;
      tu2 = tmp;
      tu1->type = TUMBLER_RANGE;
      tu2->type = TUMBLER_SINGLE;
    }
    else if (tu1->entry[DAY] > tu2->entry[DAY])
    {
      gd.f.reverse = TRUE;
      tmp = tu1;
      tu1 = tu2;
      tu2 = tmp;
      tu1->type = TUMBLER_RANGE;
      tu2->type = TUMBLER_SINGLE;
    }
    else if (tu1->entry[PART] > tu2->entry[PART])
    {
      gd.f.reverse = TRUE;
      tmp = tu1;
      tu1 = tu2;
      tu2 = tmp;
      tu1->type = TUMBLER_RANGE;
      tu2->type = TUMBLER_SINGLE;
    }
  }

  /*--------------------------------------------------------------
  ; I junked the use of struct tm as it was just too error
  ; prone.  I defined a struct btm with only the fields I need,
  ; and with the definitions I required.
  ;
  ; The tumbler code will set unspecified fields to 0.  In this 
  ; case, we want to start with minimum legal values.
  ;------------------------------------------------------------*/

  nu1         = PART;
  start.part  = (tu1->entry[PART]  == 0) ? nu1 = DAY     , 1             : tu1->entry[PART];
  start.day   = (tu1->entry[DAY]   == 0) ? nu1 = MONTH   , 1             : tu1->entry[DAY];
  start.month = (tu1->entry[MONTH] == 0) ? nu1 = YEAR    , 1             : tu1->entry[MONTH];
  start.year  = (tu1->entry[YEAR]  == 0) ? nu1 = YEAR    , gd.begin.year : tu1->entry[YEAR];
  
  if (start.day > max_monthday(start.year,start.month))
    return(1);				/* invalid day */

  /*----------------------------------------------------------
  ; Make some things easier if for a single request, we set a 
  ; range and let the result fall out of normal processing.
  ;-----------------------------------------------------------*/
  
  if (tu1->type == TUMBLER_SINGLE)
  {
    gd.f.navigation = TRUE;
    nu2        = PART;
    end.year   = (tu1->entry[YEAR]  == 0) ? nu2 = YEAR  , gd.now.year                      : tu1->entry[YEAR];
    end.month  = (tu1->entry[MONTH] == 0) ? nu2 = MONTH , 12                               : tu1->entry[MONTH];
    end.day    = (tu1->entry[DAY]   == 0) ? nu2 = DAY   , max_monthday(end.year,end.month) : tu1->entry[DAY];
    end.part   = (tu1->entry[PART]  == 0) ? nu2 = PART  , 23                               : tu1->entry[PART];
    gd.navunit = nu1;
    
    calculate_previous(start);
    calculate_next(end);
  }
  else
  {
    /*-----------------------------------------------------------
    ; fix the ending date.  In this case, the unspecified fields
    ; will be set to the their maximum legal value.
    ;-----------------------------------------------------------*/
    
    end.year  = (tu2->entry[YEAR]  == 0) ? gd.now.year                      : tu2->entry[YEAR];
    end.month = (tu2->entry[MONTH] == 0) ? 12                               : tu2->entry[MONTH];
    end.day   = (tu2->entry[DAY]   == 0) ? max_monthday(end.year,end.month) : tu2->entry[DAY];
    end.part  = (tu2->entry[PART]  == 0) ? 23                               : tu2->entry[PART];
  }

  if (end.day > max_monthday(end.year,end.month))
    return(1);

  /*------------------------------------------------
  ; Okay, sanity checking here ... 
  ; These should be true once we hit this spot of the
  ; code.
  ;------------------------------------------------*/
  
  ddt(start.year  >= 1);
  ddt(start.month >= 1);
  ddt(start.month <= 12);
  ddt(start.day   >= 1);
  ddt(start.day   <= max_monthday(start.year,start.month));
  
  ddt(end.year  >= 1);
  ddt(end.month >= 1);
  ddt(end.month <= 12);
  ddt(end.day   >= 1);
  ddt(end.day   <= max_monthday(end.year,end.month));
  
  /*--------------------------------------------------------------
  ; okay, resume processing ... bound against the starting time of
  ; the blog, and the current time.
  ;-------------------------------------------------------------*/
  
  if (btm_cmp(&start,&gd.begin) < 0)
    start = gd.begin;
  if (btm_cmp(&end,&gd.now) > 0)
    end = gd.now;
  
  /*----------------------------------------------------
  ; From here on out, it's pretty straight forward.
  ; read a day, if it has entries, add it to the list, 
  ; otherwise, continue on, advancing (or retreading) the
  ; days as we go ... 
  ;-------------------------------------------------------*/

  /*----------------------------------------------------
  ; these four lines replaced 65 very confused lines
  ; of code.
  ;----------------------------------------------------*/
  
  if (gd.f.reverse)
    BlogEntryReadBetweenD(g_blog,&cbd.list,&end,&start);
  else
    BlogEntryReadBetweenU(g_blog,&cbd.list,&start,&end);
  
  tags     = tag_collect(&cbd.list);  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);

  return(0);
}

/******************************************************************/

static void calculate_previous(struct btm start)
{
  gd.previous = start;

  switch(gd.navunit)
  {
    case YEAR:
         if (start.year == gd.begin.year)
           gd.f.navprev = FALSE;
         else
           gd.previous.year = start.year - 1;
         break;
    case MONTH:
         if (
              (start.year == gd.begin.year) 
              && (start.month == gd.begin.month)
            )
           gd.f.navprev = FALSE;
         else
	   btm_sub_month(&gd.previous);
         break;
    case DAY:
         if (btm_cmp_date(&start,&gd.begin) == 0)
           gd.f.navprev = FALSE;
         else
         {
           btm_sub_day(&gd.previous);
           
           while(btm_cmp(&gd.previous,&gd.begin) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
               btm_sub_day(&gd.previous);
               continue;
             }
             
             return;
           }
                      
           gd.f.navprev = FALSE;
         }
         break;
    case PART:
         if (btm_cmp(&start,&gd.begin) == 0)
           gd.f.navprev = FALSE;
         else
         {
	   btm_sub_part(&gd.previous);

           while(btm_cmp(&gd.previous,&gd.begin) > 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.previous);
             if (entry == NULL)
             {
	       btm_sub_part(&gd.previous);
               continue;
             }
             
             return;
           }
           
           gd.f.navprev = FALSE;
         }
         break;
    default:
         ddt(0);
  }
}

/******************************************************************/

static void calculate_next(struct btm end)
{
  gd.next = end;
  
  switch(gd.navunit)
  {
    case YEAR:
         if (end.year == gd.now.year)
           gd.f.navnext = FALSE;
         else
           gd.next.year  = end.year + 1;
         break;
    case MONTH:
         if (
              (end.year == gd.now.year) 
              && (end.month == gd.now.month)
            )
           gd.f.navnext = FALSE;
         else
           btm_add_month(&gd.next);
         break;
    case DAY:
         if (btm_cmp_date(&end,&gd.now) == 0)
           gd.f.navnext = FALSE;
         else
         {
           btm_add_day(&gd.next);
	   gd.next.part = 1;
           
           while(btm_cmp(&gd.next,&gd.now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               btm_add_day(&gd.next);
               continue;
             }
             
             return;
           }
           
           gd.f.navnext = FALSE;
         }
         break;
    case PART:
         if (btm_cmp(&end,&gd.now) == 0)
           gd.f.navnext = FALSE;
	 else
	 {
	   gd.next.part++;
         
           while(btm_cmp(&gd.next,&gd.now) <= 0)
           {
             BlogEntry entry;
             
             entry = BlogEntryRead(g_blog,&gd.next);
             if (entry == NULL)
             {
               gd.next.part = 1;
               btm_add_day(&gd.next);
               continue;
             }
             
             return;
           }
           gd.f.navnext = FALSE;
	 }
         break;
    default:
         ddt(0);
  }
}

/******************************************************************/

static const char *mime_type(char *filename)
{
  int                   i;
  static struct dstring types[] = 
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
    { NULL		, NULL		}
  };
  
  ddt(filename != NULL);
  
  for (i = 0 ; types[i].s1 != NULL ; i++)
  {
    if (strstr(filename,types[i].s1) != NULL)
      return(types[i].s2);
  }
  
  return("text/plain");
}

/******************************************************************/

static int display_file(Stream out,Tumbler spec)
{
  TumblerUnit tu;
  char        fname[FILENAME_MAX];
  
  ddt(out  != NULL);
  ddt(spec != NULL);

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
    Stream       in;
    const char  *type;
    int          rc;
    
    rc = stat(fname,&status);
    if (rc == -1) 
    {
      if (errno == ENOENT)
        (*gd.req->error)(gd.req,HTTP_NOTFOUND,"$ $","%a: %b",fname,strerror(errno));
      else if (errno == EACCES)
        (*gd.req->error)(gd.req,HTTP_FORBIDDEN,"$ $","%a: %b",fname,strerror(errno));
      else
        (*gd.req->error)(gd.req,HTTP_ISERVERERR,"$ $","%a: %b",fname,strerror(errno));
      return(1);
    }

    in = FileStreamRead(fname);    
    if (in == NULL)
    {
      (*gd.req->error)(gd.req,HTTP_NOTFOUND,"$","%a: some internal error",fname);
      return(1);
    }
    
    type = mime_type(tu->file);

    if (strcmp(type,"text/x-html") == 0)
    {
      gd.htmldump = in;
      LineS(out,"Status: 200\r\nContent-type: text/html\r\n\r\n");
      generic_cb("main",out,NULL);
    }
    else
    {
      LineSFormat(
      		out,
      		"$ L",
		"Status: 200\r\n"
                "Content-type: %a\r\n"
  	        "Content-length: %b\r\n"
		"\r\n",
	        type,
	        (unsigned long)status.st_size
	    );
    
      StreamCopy(out,in);
    }
    StreamFree(in);    
  }
  else
  {
    LineSFormat(out,"$","File to open: %a\n",fname);
  }
  return(ERR_OKAY);
}

/*****************************************************************/

int primary_page(Stream out,int year,int month,int iday,int part)
{
  BlogEntry             entry;
  struct btm            thisday;
  int                   days;
  char                 *tags;
  struct callback_data  cbd;
  int                   added;
  
  ddt(out   != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  gd.f.fullurl = FALSE;
  gd.f.reverse = TRUE;
    
  thisday.year  = year;
  thisday.month = month;
  thisday.day   = iday;
  thisday.part  = part;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  for (ListInit(&cbd.list) , days = 0 , added = FALSE ; days < c_days ; )
  {
    if (btm_cmp(&thisday,&gd.begin) < 0) break;
    
    entry = BlogEntryRead(g_blog,&thisday);
    if (entry)
    {
      ListAddTail(&cbd.list,&entry->node);
      added = TRUE;
    }

    thisday.part--;
    if (thisday.part == 0)
    {
      thisday.part = 23;
      btm_sub_day(&thisday);
      if (added)
        days++;
      added = FALSE;
    }
  }

  tags = tag_collect(&cbd.list);
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);
  
  return(0);
}

/********************************************************************/

static int rss_page(Stream out,struct btm *when,int fullurl,int reverse)
{
  struct btm            thisday;
  char                 *tags;
  struct callback_data  cbd;
  
  ddt(out   != NULL);
  ddt(when  != NULL);
  
  gd.f.fullurl = fullurl;
  gd.f.reverse = reverse;
  thisday      = *when;
  
  memset(&cbd,0,sizeof(struct callback_data));
  
  ListInit(&cbd.list);
  if (gd.f.reverse)
    BlogEntryReadXD(g_blog,&cbd.list,&thisday,c_rssitems);
  else
    BlogEntryReadXU(g_blog,&cbd.list,&thisday,c_rssitems);

  tags     = tag_collect(&cbd.list);  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&cbd);
  free_entries(&cbd.list);

  return(0);
}

/********************************************************************/

static char *tag_collect(List *list)
{
  Stream     stags;
  BlogEntry  entry;
  char      *comma = "";
  char      *tags;
  
  stags = StringStreamWrite();
  
  for
  (
    entry = (BlogEntry)ListGetHead(list);
    NodeValid(&entry->node);
    entry = (BlogEntry)NodeNext(&entry->node)
  )
  {
    if (empty_string(entry->class)) continue;
    LineS(stags,comma);
    LineS(stags,entry->class);
    comma = ", ";
  }
  
  tags = StringFromStream(stags);
  StreamFree(stags);
  return(tags);
}

/********************************************************************/

static char *tag_pick(const char *tag)
{
  String *pool;
  size_t  num;
  size_t  r;
  char   *pick;
  
  ddt(tag != NULL);

  if (empty_string(tag))
    return(dup_string(gd.adtag));
  
  pool = tag_split(&num,tag);

  /*-------------------------------------
  ; if num is 0, then the tag string was
  ; malformed (basically, started with
  ; a ',') and therefore, we fall back to
  ; the default adtag.
  ;-------------------------------------*/

  if (num)
  {
    r  = (((double)rand() / (double)RAND_MAX) * (double)num); 
    ddt(r < num);
    pick = fromstring(pool[r]);
  }
  else
    pick = dup_string(gd.adtag);
  
  MemFree(pool);
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
    BlogEntryFree(entry);
  }
}

/******************************************************************/

