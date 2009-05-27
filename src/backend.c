
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

#include <cgil/memory.h>
#include <cgil/ddt.h>
#include <cgil/htmltok.h>
#include <cgil/buffer.h>
#include <cgil/clean.h>
#include <cgil/util.h>
#include <cgil/cgi.h>

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

void init_display(Display d)
{
  assert(d != NULL);

  memset(d,0,sizeof(struct display));
  d->callbacks = m_callbacks;
  d->numcb     = CALLBACKS;
  d->navunit   = INDEX;
  d->f.navprev = TRUE;
  d->f.navnext = TRUE;
}

/**************************************************************************/

static int generate_pages(Request req,Display d)
{
  FILE *rssf;
  int   rc = 0;

  ddt(req != NULL);
  ddt(d   != NULL);
  
  rssf = fopen(g_daypage,"w");
  if (rssf == NULL) 
  {
    ErrorPush(AppErr,APPERR_GENPAGE,APPERR_GENPAGE,"$",g_daypage);
    ErrorLog();
    return(1);
  }
  
  rc = primary_page(rssf,m_now.tm_year,m_now.tm_mon,m_now.tm_mday);
  fclose(rssf);
    
  if (g_rsstemplates)
  {
    g_templates = g_rsstemplates;
    rssf = fopen(g_rssfile,"w");
    if (rssf == NULL)
    {
      ErrorPush(AppErr,APPERR_GENPAGE,APPERR_GENPAGE,"$",g_rssfile);
      ErrorLog();
      return(1);
    }
    rc = rss_page(rssf,m_now.tm_year,m_now.tm_mon,m_now.tm_mday);
    fclose(rssf);
  }

  if (g_tabtemplates)
  {
    g_templates = g_tabtemplates;
    rssf = fopen(g_tabfile,"w");
    if (rssf == NULL)
    {
      ErrorPush(AppErr,APPERR_GENPAGE,APPERR_GENPAGE,"$",g_tabtemplates);
      ErrorLog();
      return(1);
    }
    
    rc = tab_page(rssf,m_now.tm_year,m_now.tm_mon,m_now.tm_mday);
    fclose(rssf);
  }
  return(rc);
}

/******************************************************************/

static int tumbler_page(FILE *fpout,Tumbler spec)
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
  void        (*addday) (struct tm *)             = (day_add);
  
  ddt(fpout != NULL);
  ddt(spec  != NULL);
  
  m_fullurl = FALSE;
  
  /*---------------------------------------------------------
  ; easy checks first.  Get these out of the way ... 
  ;---------------------------------------------------------*/
  
  if (spec->flags.error)
  {
    if (m_cgi)
      display_error(404);
    else
      fprintf(stderr,"404 not found\n");
      
    return(1);
  }
  
  if (spec->flags.redirect)
  {
    if (m_cgi)
      display_error(501);
    else
      fprintf(stderr,"redirect, work out later\n");
      
    return(0);
  }
  
  if (spec->flags.file)
  {
    return(display_file(spec));
  }

  if (m_cgi)
    printf("HTTP/1.0 200\r\nContent-Type: text/html\r\n\r\n");
      
  ListInit(&listodays);
  
  tm_init(&start);
  tm_init(&end);
  
  tu1 = (TumblerUnit)ListGetHead(&spec->units);
  
  /*-----------------------------------------------------
  ; validate input tumbler from user.  
  ;----------------------------------------------------*/

  if (
       (tu1->entry[YEAR]) 
       && (tu1->entry[YEAR] < (m_begin.tm_year + 1900))
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
  start.tm_year = (tu1->entry[YEAR]  == 0) ? nu1 = YEAR    , m_begin.tm_year + 1900 : (nu1 = YEAR  , tu1->entry[YEAR]);
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
    m_navigation = TRUE;
    nu2          = PART;
    end.tm_year  = (tu1->entry[YEAR]  == 0) ? nu2 = YEAR  , m_now.tm_year + 1900 : tu1->entry[YEAR];
    end.tm_mon   = (tu1->entry[MONTH] == 0) ? nu2 = MONTH , 12 : tu1->entry[MONTH];
    end.tm_mday  = (tu1->entry[DAY]   == 0) ? nu2 = DAY   , max_monthday(end.tm_year,end.tm_mon) : tu1->entry[DAY];
    end.tm_hour  = (tu1->entry[PART]  == 0) ? nu2 = PART  , 23 : tu1->entry[PART];
    m_navunit    = nu1;
    
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
    
    end.tm_year = (tu2->entry[YEAR]  == 0) ? m_now.tm_year + 1900 : tu2->entry[YEAR];
    end.tm_mon  = (tu2->entry[MONTH] == 0) ? 12 : tu2->entry[MONTH];
    end.tm_mday = (tu2->entry[DAY]   == 0) ? max_monthday(end.tm_year,end.tm_mon) : tu2->entry[DAY];
    end.tm_hour = (tu2->entry[PART]  == 0) ? 23 : tu2->entry[PART];

    /*-------------------------------------------------------
    ; validate the second part of the tumbler from the user
    ;-------------------------------------------------------*/
    
    if (
         (tu2->entry[YEAR] != 0)
         && (tu2->entry[YEAR] < (m_begin.tm_year + 1900))
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
    m_reverse     = TRUE;
    addday        = (day_sub);
  }
  
  if (tm_cmp(&start,&m_begin) < 0)
    start = m_begin;
  if (tm_cmp(&end,&m_now) > 0)
    end = m_now;
  
  /*----------------------------------------------------
  ; From here on out, it's pretty straight forward.
  ; read a day, if it has entries, add it to the list, 
  ; otherwise, continue on, advancing (or retreading) the
  ; days as we go ... 
  ;-------------------------------------------------------*/

  for (tmpdays = days = labs(days_between(&end,&start)) + 1 , thisday = start ; days > 0 ; days--)
  {
    int rc = BlogDayRead(&blog,&thisday);

    if (rc != ERR_OKAY)
    {
      ErrorClear();
      continue;
    }
    
    /*------------------------------------------------------------
    ; adjustments for partial days.  The code is ugly because
    ; of the reverse hacks I've done without really understanding
    ; how I actually *implemented* the reverse display of entries.
    ; Funny how I'm the one writting the code and even *I* don't
    ; fully understand what I'm doing here. 8-)
    ;-------------------------------------------------------------*/
    
    if (days == tmpdays)
    {
      if (m_reverse)
      {
        if (start.tm_hour <= blog->endentry)
	  blog->endentry = start.tm_hour - 1;
      }
      else
        blog->stentry = start.tm_hour - 1;
    }

    if (days == 1)		/* because we *can* specify one day ... */
    {
      if (m_reverse)
        blog->stentry = end.tm_hour - 1;
      else
      {
        if (end.tm_hour <= blog->endentry)
          blog->endentry = end.tm_hour - 1;
      }
    }
    
    /*------------------------------------------------------
    ; the rest of this should be pretty easy to understand.
    ; I hope.
    ;-------------------------------------------------------*/
    
    if (blog->number) 
      ListAddTail(&listodays,&blog->node);
    else
      BlogDayFree(&blog);

    (*addday)(&thisday);
  }
  
  generic_cb("main",fpout,&listodays);
  
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
  
  tm_init(&m_previous);
  m_previous.tm_mday = 1;
  m_previous.tm_hour = 1;

  switch(m_navunit)
  {
    case YEAR:
         if (start.tm_year == m_begin.tm_year)
           m_navprev = FALSE;
         else
           m_previous.tm_year  = start.tm_year - 1;
         break;
    case MONTH:
         if (
              (start.tm_year == m_begin.tm_year) 
              && (start.tm_mon == m_begin.tm_mon)
            )
           m_navprev = FALSE;
         else
         {
           m_previous.tm_year  = start.tm_year;
           m_previous.tm_mon   = start.tm_mon;
           month_sub(&m_previous);
         }
         break;
    case DAY:
         if (
              (start.tm_year == m_begin.tm_year) 
              && (start.tm_mon == m_begin.tm_mon) 
              && (start.tm_mday == m_begin.tm_mday)
            )
           m_navprev = FALSE;
         else
         {
           m_previous.tm_year  = start.tm_year;
           m_previous.tm_mon   = start.tm_mon;
           m_previous.tm_mday  = start.tm_mday;
           day_sub(&m_previous);
           
           while(TRUE)
           {
             BlogDay day;
             int     rc;
             
             mktime(&m_previous);
             if (tm_cmp(&m_previous,&m_begin) <= 0)
               break;
               
             rc = BlogDayRead(&day,&m_previous);
             if (rc != ERR_OKAY)
             {
               ErrorClear();
               day_sub(&m_previous);
               continue;
             }
             if (day->number)
             {
               BlogDayFree(&day);
               return;
             }

             BlogDayFree(&day);
             day_sub(&m_previous);
           }
           m_navprev = FALSE;
         }
         break;
    case PART:
         if (
	      (start.tm_year    == m_begin.tm_year) 
	      && (start.tm_mon  == m_begin.tm_mon) 
	      && (start.tm_mday == m_begin.tm_mday) 
	      && (start.tm_hour == m_begin.tm_hour)
	    )
           m_navprev = FALSE;
         else
         {
           m_previous.tm_year  = start.tm_year;
           m_previous.tm_mon   = start.tm_mon;
           m_previous.tm_mday  = start.tm_mday;
           m_previous.tm_hour  = start.tm_hour - 1;
           
           if (start.tm_hour <= 1)
           {
             day_sub(&m_previous);
             m_previous.tm_hour = 23;
           }
           
           while(tm_cmp(&m_previous,&m_begin) > 0)
           {
             BlogDay day;
             int     rc;
           
             mktime(&m_previous);
             rc = BlogDayRead(&day,&m_previous);
             if (rc != ERR_OKAY)
             {
               ErrorClear();
               day_sub(&m_previous);
               continue;
             }
           
	     if (day->number == 0)
	     {
	       day_sub(&m_previous);
	       continue;
	     }

             if (m_previous.tm_hour > day->number)
               m_previous.tm_hour = day->number;
	     if (m_previous.tm_hour == 0) m_previous.tm_hour = 1;
             BlogDayFree(&day);
             break;
           }
         }
         break;
    default:
         ddt(0);
  }
  mktime(&m_previous);
}

/******************************************************************/

static void calculate_next(struct tm end)
{
  tm_to_tm(&end);

  tm_init(&m_next);
  m_next.tm_mday = 1;
  m_next.tm_hour = 1;
    
  switch(m_navunit)
  {
    case YEAR:
         if (end.tm_year == m_now.tm_year)
           m_navnext = FALSE;
         else
           m_next.tm_year  = end.tm_year + 1;
         break;
    case MONTH:
         if (
              (end.tm_year == m_now.tm_year) 
              && (end.tm_mon == m_now.tm_mon)
            )
           m_navnext = FALSE;
         else
         {
           m_next.tm_year  = end.tm_year;
           m_next.tm_mon   = end.tm_mon;
           month_add(&m_next);
         }
         break;
    case DAY:
         if (
              (end.tm_year == m_now.tm_year) 
              && (end.tm_mon == m_now.tm_mon) 
              && (end.tm_mday == m_now.tm_mday)
            )
           m_navnext = FALSE;
         else
         {
           m_next.tm_year  = end.tm_year;
           m_next.tm_mon   = end.tm_mon;
           m_next.tm_mday  = end.tm_mday;
           day_add(&m_next);
           
           while(TRUE)
           {
             BlogDay day;
             int     rc;
             
             mktime(&m_next);
             if (tm_cmp(&m_next,&m_now) > 0)
               break;
             rc = BlogDayRead(&day,&m_next);
             if (rc != ERR_OKAY)
             {
               ErrorClear();
               day_add(&m_next);
               continue;
             }
             if (day->number)
             {
               BlogDayFree(&day);
               return;
             }
             BlogDayFree(&day);
             day_add(&m_next);
           }
           m_navnext = FALSE;
         }
         break;
    case PART:
         if (
              (end.tm_year == m_now.tm_year) 
              && (end.tm_mon == m_now.tm_mon) 
              && (end.tm_mday == m_now.tm_mday) 
              && (end.tm_hour == m_now.tm_hour)
            )
           m_navnext = FALSE;
	 else
	 {
           m_next.tm_year  = end.tm_year;
           m_next.tm_mon   = end.tm_mon;
           m_next.tm_mday  = end.tm_mday;
           m_next.tm_hour  = end.tm_hour + 1;
         
           if (end.tm_hour > 23)
           {
             day_add(&m_next);
             m_next.tm_hour = 1;
           }
         
           while(TRUE)
           {
             BlogDay day;
             int     rc;
           
             mktime(&m_next);
             if (tm_cmp(&m_next,&m_now) >= 0)
               break;
             rc = BlogDayRead(&day,&m_next);
             if (rc != ERR_OKAY)
             {
               ErrorClear();
               day_add(&m_next);
               continue;
             }
           
             if (m_next.tm_hour > day->number)
             {
               day_add(&m_next);
               m_next.tm_hour = 1;
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
  mktime(&m_next);
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

static int display_file(Tumbler spec)
{
  TumblerUnit tu;
  char        fname[FILENAME_MAX];
  
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
         
  if (m_cgi)
  {
    struct stat  status;
    FILE        *fp;
    const char  *type;
    char         buffer[BUFSIZ];
    int          rc;
    
    rc = stat(fname,&status);
    if (rc == -1) 
    {
      if (errno == ENOENT)
        display_error(404);
      else if (errno == EACCES)
        display_error(403);
      else
        display_error(500);
      return(1);
    }
    
    fp = fopen(fname,"rb");
    if (fp == NULL)
    {
      display_error(404);
      return(1);
    }
    
    type = mime_type(tu->file);

    if (strcmp(type,"text/x-html") == 0)
    {
      m_htmldump = fp;
      printf("HTTP/1.0 200\r\nContent-Type: text/html\r\n\r\n");
      generic_cb("main",stdout,NULL);
    }
    else
    {
      printf(
              "HTTP/1.0 200\r\n"
              "Content-Type: %s\r\n"
  	      "Content-length: %lu\r\n\r\n",
	      type,
	      (unsigned long)status.st_size
	    );
    
      while(1)
      {
        size_t s;
      
        s = fread(buffer,sizeof(char),BUFSIZ,fp);
        if (s == 0) break;
        fwrite(buffer,sizeof(char),s,stdout);
      }
    }
    
    fclose(fp);
  }
  else
  {
    printf("File to open: %s\n",fname);
  }
  return(ERR_OKAY);
}

/*****************************************************************/

static int primary_page(FILE *fpout,int year,int month,int iday)
{
  BlogDay   day;
  struct tm thisday;
  int       days;
  List      listodays;
  
  ddt(fpout != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  m_fullurl = FALSE;
  
  if (m_cgi)
    printf("HTTP/1.0 200\r\nContent-Type: text/html\r\n\r\n");
  
  m_reverse = TRUE;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  thisday.tm_hour = 1;
  
  for (ListInit(&listodays) , days = 0 ; days < g_days ; )
  {
    int rc;
    
    if (tm_cmp(&thisday,&m_begin) < 0) break;
    if (tm_cmp(&thisday,&m_now)   > 0) break;

    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
    {
      ErrorLog();
      ErrorClear();
      continue;
    }
    
    if (day->number)
    {
      ListAddTail(&listodays,&day->node);
      days++;
    }
      else
        BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  generic_cb("main",fpout,&listodays);

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

static int rss_page(FILE *fpout,int year, int month, int iday)
{
  BlogDay   day;
  struct tm thisday;
  int       items;
  List      listodays;
  
  ddt(fpout != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  m_fullurl = TRUE;
  m_reverse = g_rssreverse;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  
  for (ListInit(&listodays) , items = 0 ; items < g_rssitems ; )
  {
    int rc;
    
    if (tm_cmp(&thisday,&m_begin) < 0) break;
    if (tm_cmp(&thisday,&m_now)   > 0) break;
    
    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
    {
      ErrorLog();
      ErrorClear();
      return(1);
      continue;			/* what was I thinking? */
    }
    
    if (day->number)
    {
      if (m_reverse)
        ListAddTail(&listodays,&day->node);
      else
        ListAddHead(&listodays,&day->node);
      items += day->number;
    }
      else
        BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  generic_cb("main",fpout,&listodays);

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

static int tab_page(FILE *fpout,int year, int month, int iday)
{
  BlogDay   day;
  struct tm thisday;
  int       items;
  List      listodays;
  
  ddt(fpout != NULL);
  ddt(year  >  0);
  ddt(month >  0);
  ddt(month <  13);
  ddt(iday  >  0);
  ddt(iday  <= max_monthday(year,month));

  m_fullurl = FALSE;
  m_reverse = g_tabreverse;
    
  tm_init(&thisday);

  thisday.tm_year = year;
  thisday.tm_mon  = month;
  thisday.tm_mday = iday;
  
  for (ListInit(&listodays) , items = 0 ; items < g_rssitems ; )
  {
    int rc;
    
    if (tm_cmp(&thisday,&m_begin) < 0) break;
    if (tm_cmp(&thisday,&m_now)   > 0) break;
    
    rc = BlogDayRead(&day,&thisday);
    
    if (rc != ERR_OKAY)
    {
      ErrorLog();
      ErrorClear();
      continue;
    }
    
    if (day->number)
    {
      if (m_reverse)
        ListAddTail(&listodays,&day->node);
      else
        ListAddHead(&listodays,&day->node);
      items += day->number;
    }
    else
      BlogDayFree(&day);

    day_sub(&thisday);
  }
  
  generic_cb("main",fpout,&listodays);

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

static void display_error(int err)
{
  fprintf(
           stdout,
           "HTTP/1.0 %d\r\n"
           "Content-type: text/html\r\n"
           "\r\n"
           "<html>\r\n"
           "<head>\r\n"
           "<title>Error %d</title>\r\n"
           "</head>\r\n"
           "<body>\r\n"
           "<h1>Error %d</h1>\r\n"
           "</body>\r\n"
           "</html>\r\n"
           "\r\n",
           err,
           err,
           err
         );
}

/*******************************************************************/

