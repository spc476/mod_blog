
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/cgi.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "timeutil.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  Cgi cgi;
  int rc;
  
  MemInit   ();
  BufferInit();
  DdtInit   ();
  CleanInit ();

  if (CgiNew(&cgi,NULL) == ERR_OKAY)
  {
    switch(CgiMethod(cgi))
    {
      case GET:  rc = main_cgi_get (cgi,argc,argv); break;
      case POST: rc = main_cgi_post(cgi,argc,argv); break;
      default:   rc = EXIT_FAILURE;
    }
  }
  else
    rc = main_cli(argc,argv);
  
  return(rc);
}

/***********************************************************************/

static struct tm m_begin;
static struct tm m_now;
static struct tm m_updatetime;

void BlogDatesInit(void)
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

/***********************************************************************/

