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
  
  while(gf_debug)
    ;

  MemInit   ();
  DdtInit   ();
  StreamInit();

  if (CgiNew(&cgi,NULL) == ERR_OKAY)
  {
    gd.cgi = cgi;

    switch(CgiMethod(cgi))
    {
      case GET:  rc = main_cgi_get (cgi,argc,argv); break;
      case POST: rc = main_cgi_post(cgi,argc,argv); break;
      case HEAD: rc = main_cgi_head(cgi,argc,argv); break;
      default:   rc = EXIT_FAILURE;
    }
  }
  else
    rc = main_cli(argc,argv);

  StreamFlush(StderrStream);
  StreamFlush(StdoutStream);
  return(rc);
}

/***********************************************************************/

int BlogDatesInit(void)
{
  BlogEntry  entry;
  struct tm *today;
  
  gd.tst       = time(NULL);
  today        = localtime(&gd.tst);
  gd.stmst     = *today;
  gd.now.year  = gd.updatetime.year  = today->tm_year + 1900;
  gd.now.month = gd.updatetime.month = today->tm_mon + 1;
  gd.now.day   = gd.updatetime.day   = today->tm_mday;
  gd.now.part  = gd.updatetime.part  = 0;
  srand(gd.tst);

  while(TRUE)
  {
    if (btm_cmp_date(&gd.now,&gd.begin) < 0)
      return(ERR_OKAY);	/* XXX - this may not be an error */
    
    entry = BlogEntryRead(g_blog,&gd.now);
    if (entry == NULL)
    {
      btm_sub_day(&gd.now);
      continue;
    }

    break;
  }
  gd.now.part = g_blog->idx;
  return(ERR_OKAY);
}
  
/***********************************************************************/

