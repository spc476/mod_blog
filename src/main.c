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
#include <stdarg.h>

#include <syslog.h>
#include <cgilib6/cgi.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "fix.h"
#include "backend.h"
#include "globals.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  Cgi cgi;
  int rc;
  
  while(gf_debug)
    ;

  cgi = CgiNew(NULL);
  
  if (cgi != NULL)
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

  return(rc);
}

/***********************************************************************/

int BlogDatesInit(void)
{
  FILE      *fp;
  time_t     now;
  struct tm *ptm;
  char       buffer[128];
  char      *p;
  
  now                 = time(NULL);
  ptm                 = localtime(&now);
  gd.updatetime.year  = ptm->tm_year + 1900;
  gd.updatetime.month = ptm->tm_mon + 1;
  gd.updatetime.day   = ptm->tm_mday;
  gd.updatetime.part  = 1;
  
  fp = fopen(".first","r");
  if (fp == NULL)
  {
    syslog(LOG_DEBUG,".first does not exist");
    gd.begin = gd.updatetime;
  }
  else
  {
    fgets(buffer,sizeof(buffer),fp);
    gd.begin.year  = strtoul(buffer,&p,10); p++;
    gd.begin.month = strtoul(p,&p,10); p++;
    gd.begin.day   = strtoul(p,&p,10); p++;
    gd.begin.part  = strtoul(p,&p,10);
    fclose(fp);
  }
  
  fp = fopen(".last","r");
  if (fp == NULL)
  {
    syslog(LOG_DEBUG,".last does not exist");
    gd.now = gd.updatetime;
  }
  else
  {
    fgets(buffer,sizeof(buffer),fp);
    gd.now.year  = strtoul(buffer,&p,10); p++;
    gd.now.month = strtoul(p,&p,10); p++;
    gd.now.day   = strtoul(p,&p,10); p++;
    gd.now.part  = strtoul(p,&p,10);
    fclose(fp);
  }
  
  return 0;
}
  
/***********************************************************************/

