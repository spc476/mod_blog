/************************************************************************
*
* Copyright 2005 by Sean Conner.  All Rights Reserved.
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
*************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/cgi.h>

#include "conf.h"
#include "globals.h"
#include "blog.h"
#include "timeutil.h"
#include "fix.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  Cgi cgi;
  int rc;
  
  while(cf_debug)
    ;

  MemInit   ();
  DdtInit   ();
  StreamInit();

  srand(time(NULL));

  if (CgiNew(&cgi,NULL) == ERR_OKAY)
  {
    gd.cgi = cgi;

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

int BlogDatesInit(void)
{
  BlogDay day;
  int     rc;

  tm_init(&gd.begin);
  gd.begin.tm_year = c_styear;
  gd.begin.tm_mon  = c_stmon;
  gd.begin.tm_mday = c_stday;
  gd.begin.tm_hour = 1;
  
  tm_to_tm(&gd.begin);
  set_time();
  
  while(TRUE)
  {
    if (tm_cmp(&gd.now,&gd.begin) < 0)
      return(ERR_ERR);	/* XXX - this may not be an error */
    
    rc = BlogDayRead(&day,&gd.now);
    if (rc != ERR_OKAY)
    {
      day_sub(&gd.now);
      continue;
    }
    
    if (day->number == 0)
    {
      BlogDayFree(&day);
      day_sub(&gd.now);
      continue;
    }
    
    gd.now.tm_hour = day->number;
    if (gd.now.tm_hour == 0) gd.now.tm_hour = 1;
    BlogDayFree(&day);
    break;
  }
  return(ERR_OKAY);
}

/***********************************************************************/

void set_time(void)
{
  time_t     t;
  struct tm *today;
  
  t      = time(NULL);
  today  = localtime(&t);
  gd.now = gd.updatetime = *today;
}

/***********************************************************************/

