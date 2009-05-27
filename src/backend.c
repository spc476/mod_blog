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

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "chunk.h"
#include "wbtum.h"
#include "frontend.h"
#include "fix.h"
#include "globals.h"
#include "blogutil.h"

#define max(a,b)	((a) > (b)) ? (a) : (b)

/*****************************************************************/

static void	   calculate_previous		(struct tm);
static void	   calculate_next		(struct tm);
static const char *mime_type			(char *);
static int	   display_file			(Stream,Tumbler);
static int	   rss_page			(Stream,int,int,int);
static int	   tab_page			(Stream,int,int,int);
static void	   display_error		(Stream,int);
static void        tag_collect			(char **ptag,BlogDay);
static char	  *tag_pick                     (const char *);

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
  rc = primary_page(out,gd.now.tm_year,gd.now.tm_mon,gd.now.tm_mday);
  StreamFree(out);
    
  if (c_rsstemplates)
  {
    g_templates = c_rsstemplates;
    out         = FileStreamWrite(c_rssfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = rss_page(out,gd.now.tm_year,gd.now.tm_mon,gd.now.tm_mday);
    StreamFree(out);
  }

  if (c_atomtemplates)
  {
    g_templates = c_atomtemplates;
    out         = FileStreamWrite(c_atomfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = rss_page(out,gd.now.tm_year,gd.now.tm_mon,gd.now.tm_mday);
    StreamFree(out);
  }
  
  if (c_tabtemplates)
  {
    g_templates = c_tabtemplates;
    out         = FileStreamWrite(c_tabfile,FILE_RECREATE);
    if (out == NULL)
      return(ERR_ERR);
    rc = tab_page(out,gd.now.tm_year,gd.now.tm_mon,gd.now.tm_mday);
    StreamFree(out);
  }
  
  return(rc);
}

/******************************************************************/

int tumbler_page(Stream out,Tumbler spec)
{
  struct tm     thisday;
  struct tm     start;
  struct tm     end;
  TumblerUnit   tu1;
  TumblerUnit   tu2;
  int           nu1;
  int           nu2;
  List          listodays;
  BlogDay       blog;
  long          days;
  long          tmpdays;
  char          *tags;
  void        (*addday) (struct tm *)             = (day_add);
  
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
    return(HTTP_CONTINUE);	/* XXX hack for now */
  }

  ListInit(&listodays);
  
  tm_init(&start);
  tm_init(&end);
  
  tu1 = (TumblerUnit)ListGetHead(&spec->units);
  
  /*-----------------------------------------------------
  ; validate input tumbler from user.  
  ;----------------------------------------------------*/

  if (
       (tu1->entry[YEAR]) 
       && (tu1->entry[YEAR] < (gd.begin.tm_year + 1900))
     ) 
    return(1);
  if (tu1->entry[MONTH] <  0) return(1);
  if (tu1->entry[MONTH] > 12) return(1);
  if ((tu1->entry[MONTH] == 0) && (tu1->entry[DAY])) return(1);
  if (tu1->entry[DAY]   <  0) return(1);
  if ((tu1->entry[MONTH]) && (tu1->entry[DAY] > max_monthday(tu1->entry[YEAR],tu1->entry[MONTH])))
    return(1);

  /*--------------------------------------------------------------
  ; Standard C libray time.h defines certains fields to
  ; have certain ranges, namely, .tm_year is years since 1900,
  ; .tm_mon is months since January.  To simplify some of 
  ; the code, we're going to use unadjusted (real world) 
  ; values, like the actual year, January is 1, etc.
  ;
  ; We'll also use .tm_hour as the part (or entry) since
  ; it's handy.  Just keep that in mind.  And in this
  ; case, we can start with 0, so no special check is needed.
  ;
  ; The tumbler code will set unspecified fields to 0.  In this 
  ; case, we want to start with minimum legal values.
  ;------------------------------------------------------------*/

  nu1           = PART;
  start.tm_year = (tu1->entry[YEAR]  == 0) ? nu1 = YEAR    , gd.begin.tm_year + 1900 : (nu1 = YEAR  , tu1->entry[YEAR]);
  start.tm_mon  = (tu1->entry[MONTH] == 0) ? nu1 = YEAR    , 1                      : (nu1 = MONTH , tu1->entry[MONTH]);
  start.tm_mday = (tu1->entry[DAY]   == 0) ? nu1 = MONTH   , 1                      : (nu1 = DAY   , tu1->entry[DAY]);
  start.tm_hour = tu1->entry[PART];
  if (tu1->entry[PART] != 0) nu1 = PART;
  
  if (start.tm_mday > max_monthday(start.tm_year,start.tm_mon))
    return(1);				/* invalid day */

  /*----------------------------------------------------------
  ; Make some things easier if for a single request, we set a 
  ; range and let the result fall out of normal processing.
  ;-----------------------------------------------------------*/
  
  if (tu1->type == TUMBLER_SINGLE)
  {
    gd.f.navigation = TRUE;
    nu2          = PART;
    end.tm_year  = (tu1->entry[YEAR]  == 0) ? nu2 = YEAR  , gd.now.tm_year + 1900 : tu1->entry[YEAR];
    end.tm_mon   = (tu1->entry[MONTH] == 0) ? nu2 = MONTH , 12 : tu1->entry[MONTH];
    end.tm_mday  = (tu1->entry[DAY]   == 0) ? nu2 = DAY   , max_monthday(end.tm_year,end.tm_mon) : tu1->entry[DAY];
    end.tm_hour  = (tu1->entry[PART]  == 0) ? nu2 = PART  , 23 : tu1->entry[PART];
    gd.navunit    = nu1;
    
    calculate_previous(start);
    calculate_next(end);
  }
  else
  {
    tu2 = (TumblerUnit)NodeNext(&tu1->node);
    
    /*-----------------------------------------------------------
    ; fix the ending date.  In this case, the unspecified fields
    ; will be set to the their maximum legal value.
    ;-----------------------------------------------------------*/
    
    end.tm_year = (tu2->entry[YEAR]  == 0) ? gd.now.tm_year + 1900 : tu2->entry[YEAR];
    end.tm_mon  = (tu2->entry[MONTH] == 0) ? 12 : tu2->entry[MONTH];
    end.tm_mday = (tu2->entry[DAY]   == 0) ? max_monthday(end.tm_year,end.tm_mon) : tu2->entry[DAY];
    end.tm_hour = (tu2->entry[PART]  == 0) ? 23 : tu2->entry[PART];

    /*-------------------------------------------------------
    ; validate the second part of the tumbler from the user
    ;-------------------------------------------------------*/
    
    if (
         (tu2->entry[YEAR] != 0)
         && (tu2->entry[YEAR] < (gd.begin.tm_year + 1900))
       )
      return(1);
    if (tu2->entry[MONTH] <  0) return(1);
    if (tu2->entry[MONTH] > 12) return(1);
    if ((tu2->entry[MONTH] == 0) && (tu2->entry[DAY])) return(1);
    if (tu2->entry[DAY]   <  0) return(1);
    if ((tu2->entry[MONTH]) && (tu2->entry[DAY]   > max_monthday(tu2->entry[YEAR],tu2->entry[MONTH])))
      return(1);
  }
  
  if (end.tm_mday > max_monthday(end.tm_year,end.tm_mon))
    return(1);

  /*------------------------------------------------
  ; Okay, sanity checking here ... 
  ; These should be true once we hit this spot of the
  ; code.
  ;------------------------------------------------*/
  
  ddt(start.tm_year >= 1);
  ddt(start.tm_mon  >= 1);
  ddt(start.tm_mon  <= 12);
  ddt(start.tm_mday >= 1);
  ddt(start.tm_mday <= max_monthday(start.tm_year,start.tm_mon));
  
  ddt(end.tm_year >= 1);
  ddt(end.tm_mon  >= 1);
  ddt(end.tm_mon  <= 12);
  ddt(end.tm_mday >= 1);
  ddt(end.tm_mday <= max_monthday(start.tm_year,start.tm_mon));
  
  /*--------------------------------------------------------------
  ; okay, resume processing ... we now have checked unnormalized
  ; times.  Normalize, then bound against the starting time of
  ; the blog, and the current time.
  ;-------------------------------------------------------------*/
  
  tm_to_tm(&start);
  tm_to_tm(&end);

  /*--------------------------------------------------
  ; if we're going in reverse, swap the times, set the
  ; reverse flag, and change the functions to add nodes
  ; in the proper direction.  My, we hacky 8-)
  ;--------------------------------------------------*/
  
  if (tm_cmp(&end,&start) < 0)
  {
    gd.f.reverse     = TRUE;
    addday        = (day_sub);
  }
  
  if (tm_cmp(&start,&gd.begin) < 0)
    start = gd.begin;
  if (tm_cmp(&end,&gd.now) > 0)
    end = gd.now;
  
  /*----------------------------------------------------
  ; From here on out, it's pretty straight forward.
  ; read a day, if it has entries, add it to the list, 
  ; otherwise, continue on, advancing (or retreading) the
  ; days as we go ... 
  ;-------------------------------------------------------*/

  tags = dup_string("");	/* an empty string to start out with */
  
  for (tmpdays = days = labs(days_between(&end,&start)) + 1 , thisday = start ; days > 0 ; days--)
  {
    int rc = BlogDayRead(&blog,&thisday);

    if (rc != ERR_OKAY)
      continue;
    
    /*------------------------------------------------------------
    ; adjustments for partial days.  The code is ugly because
    ; of the reverse hacks I've done without really understanding
    ; how I actually *implemented* the reverse display of entries.
    ; Funny how I'm the one writting the code and even *I* don't
    ; fully understand what I'm doing here. 8-)
    ;-------------------------------------------------------------*/
    
    if (days == tmpdays)
    {
      if (gd.f.reverse)
      {
        if (start.tm_hour <= blog->endentry)
	  blog->endentry = start.tm_hour - 1;
      }
      else
        blog->stentry = start.tm_hour - 1;
    }

    if (days == 1)		/* because we *can* specify one day ... */
    {
      if (gd.f.reverse)
        blog->stentry = end.tm_hour - 1;
      else
      {
        if (end.tm_hour <= blog->endentry)
          blog->endentry = end.tm_hour - 1;
      }
    }

    /*----------------------------------------------------
    ; I have no idea how long ago I wrote this code, but in
    ; all that time, I never knew that blog->stentry or 
    ; blog->endentry could become negative.  It didn't seem
    ; to affect the rest of the code, but with the new adtag
    ; feature, it became a problem.  This bit o' code ensures
    ; that they at least remain positive.  
    ;
    ; I still don't fully understand what I'm doing here.
    ;--------------------------------------------------------*/

    if (blog->stentry  < 0) blog->stentry  = 0;
    if (blog->endentry < 0) blog->endentry = 0;
    
    /*------------------------------------------------------
    ; the rest of this should be pretty easy to understand.
    ; I hope.
    ;-------------------------------------------------------*/
    
    if (blog->number)
    {
      tag_collect(&tags,blog);
      ListAddTail(&listodays,&blog->node);
    }
    else
      BlogDayFree(&blog);

    (*addday)(&thisday);
  }
  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&listodays);
  
  for (
        blog = (BlogDay)ListRemHead(&listodays);
        NodeValid(&blog->node);
        blog = (BlogDay)ListRemHead(&listodays)
      )
  {
    BlogDayFree(&blog);
  }

  return(0);
}

/******************************************************************/

static void calculate_previous(struct tm start)
{
  tm_to_tm(&start);
  
  tm_init(&gd.previous);
  gd.previous.tm_mday = 1;
  gd.previous.tm_hour = 1;

  switch(gd.navunit)
  {
    case YEAR:
         if (start.tm_year == gd.begin.tm_year)
           gd.f.navprev = FALSE;
         else
           gd.previous.tm_year  = start.tm_year - 1;
         break;
    case MONTH:
         if (
              (start.tm_year == gd.begin.tm_year) 
              && (start.tm_mon == gd.begin.tm_mon)
            )
           gd.f.navprev = FALSE;
         else
         {
           gd.previous.tm_year  = start.tm_year;
           gd.previous.tm_mon   = start.tm_mon;
           month_sub(&gd.previous);
         }
         break;
    case DAY:
         if (
              (start.tm_year == gd.begin.tm_year) 
              && (start.tm_mon == gd.begin.tm_mon) 
              && (start.tm_mday == gd.begin.tm_mday)
            )
           gd.f.navprev = FALSE;
         else
         {
           gd.previous.tm_year  = start.tm_year;
           gd.previous.tm_mon   = start.tm_mon;
           gd.previous.tm_mday  = start.tm_mday;
           day_sub(&gd.previous);
           
           while(TRUE)
           {
             BlogDay day;
             int     rc;
             
             mktime(&gd.previous);
             if (tm_cmp(&gd.previous,&gd.begin) <= 0)
               break;
               
             rc = BlogDayRead(&day,&gd.previous);
             if (rc != ERR_OKAY)
             {
               day_sub(&gd.previous);
               continue;
             }
             if (day->number)
             {
               BlogDayFree(&day);
               return;
             }

             BlogDayFree(&day);
             day_sub(&gd.previous);
           }
           gd.f.navprev = FALSE;
         }
         break;
    case PART:
         if (
	      (start.tm_year    == gd.begin.tm_year) 
	      && (start.tm_mon  == gd.begin.tm_mon) 
	      && (start.tm_mday == gd.begin.tm_mday) 
	      && (start.tm_hour == gd.begin.tm_hour)
	    )
           gd.f.navprev = FALSE;
         else
         {
           gd.previous.tm_year  = start.tm_year;
           gd.previous.tm_mon   = start.tm_mon;
           gd.previous.tm_mday  = start.tm_mday;
           gd.previous.tm_hour  = start.tm_hour - 1;
           
           if (start.tm_hour <= 1)
           {
             day_sub(&gd.previous);
             gd.previous.tm_hour = 23;
           }
           
           while(tm_cmp(&gd.previous,&gd.begin) > 0)
           {
             BlogDay day;
             int     rc;
           
             mktime(&gd.previous);
             rc = BlogDayRead(&day,&gd.previous);
             if (rc != ERR_OKAY)
             {
               day_sub(&gd.previous);
               continue;
             }
           
	     if (day->number == 0)
	     {
	       day_sub(&gd.previous);
	       continue;
	     }

             if (gd.previous.tm_hour > day->number)
               gd.previous.tm_hour = day->number;
	     if (gd.previous.tm_hour == 0) gd.previous.tm_hour = 1;
             BlogDayFree(&day);
             break;
           }
         }
         break;
    default:
         ddt(0);
  }
  mktime(&gd.previous);
}

/******************************************************************/

static void calculate_next(struct tm end)
{
  tm_to_tm(&end);

  tm_init(&gd.next);
  gd.next.tm_mday = 1;
  gd.next.tm_hour = 1;
    
  switch(gd.navunit)
  {
    case YEAR:
         if (end.tm_year == gd.now.tm_year)
           gd.f.navnext = FALSE;
         else
           gd.next.tm_year  = end.tm_year + 1;
         break;
    case MONTH:
         if (
              (end.tm_year == gd.now.tm_year) 
              && (end.tm_mon == gd.now.tm_mon)
            )
           gd.f.navnext = FALSE;
         else
         {
           gd.next.tm_year  = end.tm_year;
           gd.next.tm_mon   = end.tm_mon;
           month_add(&gd.next);
         }
         break;
    case DAY:
         if (
              (end.tm_year == gd.now.tm_year) 
              && (end.tm_mon == gd.now.tm_mon) 
              && (end.tm_mday == gd.now.tm_mday)
            )
           gd.f.navnext = FALSE;
         else
         {
           gd.next.tm_year  = end.tm_year;
           gd.next.tm_mon   = end.tm_mon;
           gd.next.tm_mday  = end.tm_mday;
           day_add(&gd.next);
           
           while(TRUE)
           {
             BlogDay day;
             int     rc;
             
             mktime(&gd.next);
             if (tm_cmp(&gd.next,&gd.now) > 0)
               break;
             rc = BlogDayRead(&day,&gd.next);
             if (rc != ERR_OKAY)
             {
               day_add(&gd.next);
               continue;
             }
             if (day->number)
             {
               BlogDayFree(&day);
               return;
             }
             BlogDayFree(&day);
             day_add(&gd.next);
           }
           gd.f.navnext = FALSE;
         }
         break;
    case PART:
         if (
              (end.tm_year == gd.now.tm_year) 
              && (end.tm_mon == gd.now.tm_mon) 
              && (end.tm_mday == gd.now.tm_mday) 
              && (end.tm_hour == gd.now.tm_hour)
            )
           gd.f.navnext = FALSE;
	 else
	 {
           gd.next.tm_year  = end.tm_year;
           gd.next.tm_mon   = end.tm_mon;
           gd.next.tm_mday  = end.tm_mday;
           gd.next.tm_hour  = end.tm_hour + 1;
         
           if (end.tm_hour > 23)
           {
             day_add(&gd.next);
             gd.next.tm_hour = 1;
           }
         
           while(TRUE)
           {
             BlogDay day;
             int     rc;
           
             mktime(&gd.next);
             if (tm_cmp(&gd.next,&gd.now) >= 0)
               break;
             rc = BlogDayRead(&day,&gd.next);
             if (rc != ERR_OKAY)
             {
               day_add(&gd.next);
               continue;
             }
           
             if (gd.next.tm_hour > day->number)
             {
               day_add(&gd.next);
               gd.next.tm_hour = 1;
               continue;
             }
             BlogDayFree(&day);
             break;
           }
	 }
         break;
    default:
         ddt(0);
  }
  mktime(&gd.next);
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
        display_error(out,404);
      else if (errno == EACCES)
        display_error(out,403);
      else
        display_error(out,500);
      return(1);
    }

    in = FileStreamRead(fname);    
    if (in == NULL)
    {
      display_error(out,404);
      return(1);
    }
    
    type = mime_type(tu->file);

    if (strcmp(type,"text/x-html") == 0)
    {
      gd.htmldump = in;
      LineS(out,"HTTP/1.0 200 Okay\r\nContent-type: text/html\r\n\r\n");
      generic_cb("main",out,NULL);
    }
    else
    {
      LineSFormat(
      		out,
      		"$ L",
                "HTTP/1.0 200 Okay\r\n"
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

int primary_page(Stream out,int year,int month,int iday)
{
  BlogDay   day;
  struct tm thisday;
  int       days;
  List      listodays;
  char     *tags;
  
  ddt(out   != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  tags         = dup_string("");
  gd.f.fullurl = FALSE;
  gd.f.reverse = TRUE;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  thisday.tm_hour = 1;
  
  for (ListInit(&listodays) , days = 0 ; days < c_days ; )
  {
    int rc;
    
    /*-----------------------------------------------------------------
    ; There's a bug.  gd.now isn't be initialized properly in all
    ; code paths.  If it *isn't* initialized properly, then 
    ;
    ;	blog --config conf 
    ;
    ; works between 00:00:00 and 00:59:59, but
    ;
    ;	blog --config conf --regen
    ;
    ; fails between 00:00:00 and 00:59:59.  If it *is* initialized
    ; properly, then both would fail between 00:00:00 and 00:59:59.
    ;
    ; This bug is due to the way I use the struct tm.tm_hour field, not
    ; as an hour per se, but as a entry to use.
    ;
    ; This is bad.
    ;
    ; I need to think on this.
    ;-------------------------------------------------------------------*/
    
    if (tm_cmp(&thisday,&gd.begin) < 0) break;
    if (tm_cmp(&thisday,&gd.now)   > 0) break;

    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
      continue;
    
    if (day->number)
    {
      tag_collect(&tags,day);
      ListAddTail(&listodays,&day->node);
      days++;
    }
      else
        BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&listodays);

  for (
        day = (BlogDay) ListRemHead(&listodays) ; 
        NodeValid(&day->node) ;
        day = (BlogDay) ListRemHead(&listodays)
      )
  {
    BlogDayFree(&day);
  }

  return(0);
}

/********************************************************************/

static int rss_page(Stream out,int year, int month, int iday)
{
  BlogDay    day;
  struct tm  thisday;
  int        items;
  List       listodays;
  char      *tags;
  
  ddt(out   != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  tags         = dup_string("");
  gd.f.fullurl = TRUE;
  gd.f.reverse = cf_rssreverse;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  
  for (ListInit(&listodays) , items = 0 ; items < c_rssitems ; )
  {
    int rc;
    
    if (tm_cmp(&thisday,&gd.begin) < 0) break;
    if (tm_cmp(&thisday,&gd.now)   > 0) break;
    
    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
      return(1);
    
    if (day->number)
    {
      tag_collect(&tags,day);
      if (gd.f.reverse)
        ListAddTail(&listodays,&day->node);
      else
        ListAddHead(&listodays,&day->node);
      items += day->number;
    }
      else
        BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&listodays);

  for (
        day = (BlogDay) ListRemHead(&listodays) ; 
        NodeValid(&day->node) ;
        day = (BlogDay) ListRemHead(&listodays)
      )
  {
    BlogDayFree(&day);
  }

  return(0);
}

/********************************************************************/

static int tab_page(Stream out,int year, int month, int iday)
{
  BlogDay    day;
  struct tm  thisday;
  int        items;
  List       listodays;
  char      *tags;
  
  ddt(out   != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  tags         = dup_string("");
  gd.f.fullurl = FALSE;
  gd.f.reverse = cf_tabreverse;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  
  for (ListInit(&listodays) , items = 0 ; items < c_rssitems ; )
  {
    int rc;
    
    if (tm_cmp(&thisday,&gd.begin) < 0) break;
    if (tm_cmp(&thisday,&gd.now)   > 0) break;
    
    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
      continue;
    
    if (day->number)
    {
      tag_collect(&tags,day);
      if (gd.f.reverse)
        ListAddTail(&listodays,&day->node);
      else
        ListAddHead(&listodays,&day->node);
      items += day->number;
    }
    else
      BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  gd.adtag = tag_pick(tags);
  MemFree(tags);
  generic_cb("main",out,&listodays);

  for (
        day = (BlogDay) ListRemHead(&listodays) ; 
        NodeValid(&day->node) ;
        day = (BlogDay) ListRemHead(&listodays)
      )
  {
    BlogDayFree(&day);
  }

  return(0);
}

/*******************************************************************/

static void display_error(Stream out,int err)
{
  LineSFormat(
  	out,
  	"i",
        "HTTP/1.0 %a Error\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<html>\n"
        "<head>\n"
        "<title>Error %a</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Error %a</h1>\n"
        "</body>\n"
        "</html>\n"
        "\n",
        err
         );
}

/*******************************************************************/

static void tag_collect(char **ptag,BlogDay blog)
{
  char   *tag;
  char   *t;
  char   *comma = "";
  size_t  i;
  
  ddt(ptag  != NULL);
  ddt(*ptag != NULL);
  ddt(blog  != NULL);
  
  tag = *ptag;
  if (*tag != '\0')
    comma = ", ";
  
  for (i = blog->stentry ; i <= blog->endentry ; i++)
  {
    if (empty_string(blog->entries[i]->class)) continue;
    t = concat_strings(tag,comma,blog->entries[i]->class,(char *)NULL);
    MemFree(tag);
    tag   = t;
    comma = ", ";
  }  
  *ptag = tag;
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
#if 0
    pick = MemAlloc(pool[r].s + 1);
    memcpy(pick,pool[r].d,pool[r].s);
    pick[pool[r].s] = '\0';
#endif
  }
  else
    pick = dup_string(gd.adtag);
  
  MemFree(pool);
  return(pick);
}
 
/******************************************************************/

