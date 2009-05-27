
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

static const char            *m_updatetype = "NewEntry";
static struct tm              m_begin;
static struct tm              m_now;
static struct tm              m_updatetime;
static struct tm              m_previous;
static struct tm              m_next;
static FILE                  *m_htmldump    = NULL;
static int                    m_fullurl     = FALSE;
static int                    m_reverse     = FALSE;
static int                    m_cgi         = FALSE;
static int                    m_navigation  = FALSE;
static int                    m_navunit     = INDEX;
static int                    m_navprev     = TRUE;
static int                    m_navnext     = TRUE;
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
 
int main(int argc,char *argv[])
{
  Cgi cgi;
  
  MemInit();
  BufferInit();
  DdtInit();
  CleanInit();
  
  if (CgiNew(&cgi,NULL) == ERR_OKAY)
    return(cgi_main(cgi,argc,argv));
  else
    return(cli_main(argc,argv));
    
  return(EXIT_FAILURE);
}

/************************************************************************/

static void BlogDatesInit(void)
{
  time_t     t;
  struct tm *today;
  BlogDay    day;
  int        rc;

  BlogInit();

  tm_init(&m_begin);
  m_begin.tm_year = g_styear;
  m_begin.tm_mon  = g_stmon;
  m_begin.tm_mday = g_stday;
  m_begin.tm_hour = 1;
  
  tm_to_tm(&m_begin);
  
  t     = time(NULL);
  today = localtime(&t);
  m_now = m_updatetime = *today;

  while(TRUE)
  {
    if (tm_cmp(&m_now,&m_begin) < 0)
    {
      ErrorPush(AppErr,1,APPERR_BLOGINIT,"");
      ErrorLog();
      exit(EXIT_FAILURE);
    }
    
    rc = BlogDayRead(&day,&m_now);
    if (rc != ERR_OKAY)
    {
      ErrorClear();
      day_sub(&m_now);
      continue;
    }
    
    if (day->number == 0)
    {
      BlogDayFree(&day);
      day_sub(&m_now);
      continue;
    }
    
    m_now.tm_hour = day->number;
    if (m_now.tm_hour == 0) m_now.tm_hour = 1;
    BlogDayFree(&day);
    break;
  }
}

/************************************************************************/

static int cgi_main(Cgi cgi,int argc,char *argv[])
{
  char *query;
  char *script;
  int   rc;

  if (CgiMethod(cgi) != GET)
  {
    display_error(405);
    return(EXIT_FAILURE);
  }

  m_cgi = TRUE;

  script = CgiEnvGet(cgi,"SCRIPT_FILENAME");
  if (!empty_string(script))
  {
    rc = GlobalsInit(script);
    if (rc != ERR_OKAY)
    {
      ErrorLog();
      display_error(500);
      return(EXIT_FAILURE);
    }
  }

  BlogDatesInit();
  query = CgiEnvGet(cgi,"PATH_INFO");
  if ((empty_string(query)) || (strcmp(query,"/") == 0))
  {
    rc = primary_page(stdout,m_now.tm_year,m_now.tm_mon,m_now.tm_mday);
  }
  else
  {
    Tumbler spec;
    
    if (!isdigit(*query)) query++;

    rc = TumblerNew(&spec,&query);
    if (rc == ERR_OKAY)
      rc = tumbler_page(stdout,spec);
    else
      display_error(404);
  }
  return(rc);
}

/************************************************************************/

static int cli_main(int argc,char *argv[])
{
  int rc;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s configurationfile [new modify template other | tumbler]\n",argv[0]);
    return(EXIT_FAILURE);
  }
  
  rc = GlobalsInit(argv[1]);
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    fprintf(stderr,"could not open configuration file %s\n",argv[1]);
    return(rc);
  }

  BlogDatesInit();
  
  if (argc == 2)
    rc = generate_pages();
  else if (argc == 3)
  {
    Tumbler spec;
    
    if (isdigit(argv[2][0]))
    {
      rc = TumblerNew(&spec,&argv[2]);
      if (rc == ERR_OKAY)
        rc = tumbler_page(stdout,spec);
      else
        fprintf(stderr,"tumbler error---nothing found\n");
    }
    else
    {
      if (strcmp(argv[2],"new") == 0)
        m_updatetype = "NewEntry";
      else if (strcmp(argv[2],"modify") == 0)
        m_updatetype = "ModifiedEntry";
      else if (strcmp(argv[2],"template") == 0)
        m_updatetype = "TemplateChange";
      else if (strcmp(argv[2],"other") == 0)
        m_updatetype = "Other";
      else
      {
        fprintf(stderr,"incorret type---one of [new modify template other]\n");
        return(1);
      }
      rc = generate_pages();
    }
  }
  
  return(rc);
}

/*****************************************************************/

static int generate_pages(void)
{
  FILE *rssf;
  int   rc = 0;

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

/*******************************************************************
*
* Dummy routines required for linking ... 
*
*******************************************************************/

void conversion		(char *a,Buffer b,Buffer c)	{ }
void text_conversion	(char *a,Buffer b,Buffer c)	{ }
void html_conversion	(char *a,Buffer b,Buffer c)	{ }
void mixed_conversion	(char *a,Buffer b,Buffer c)	{ }


