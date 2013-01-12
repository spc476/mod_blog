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
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>

#include <syslog.h>
#include <cgilib6/cgi.h>
#include <cgilib6/crashreport.h>

#include "conf.h"
#include "blog.h"
#include "timeutil.h"
#include "fix.h"
#include "backend.h"
#include "globals.h"

/************************************************************************/

int main(int argc,char *argv[],char *envp[])
{
  Cgi cgi;
  int rc;
  
  /*-----------------------------------------------------------------------
  ; due to a crash for no apparent reason, I decided to log a bit more
  ; infomration when it does happen.  Here are the possible signals I want
  ; more information from when it happens.
  ;-----------------------------------------------------------------------*/

  crashreport_with(argc,argv,envp);
  
  /*---------------------
  ; Defined in ANSI C
  ;-----------------------*/
  
  crashreport(SIGABRT);
  crashreport(SIGFPE);
  crashreport(SIGILL);
  crashreport(SIGINT);
  crashreport(SIGSEGV);

  /*---------------------------
  ; others we're interested in 
  ;----------------------------*/

#ifdef SIGBUG
  crashreport(SIGBUS);
#endif
#ifdef SIGQUIT
  crashreport(SIGQUIT);
#endif
#ifdef SIGSYS
  crashreport(SIGSYS);
#endif
#ifdef SIGEMT
  crashreport(SIGEMT);
#endif
#ifdef SIGTKFLT
  crashreport(SIGTKFLT);
#endif
  
  /*-----------------------------------------------------------------------
  ; WTF?  This isn't a WTF.  This is a debugging techique.  While gf_debug
  ; is normally set to false by default, changing it to true in the source
  ; code will cause the program to spin right here when run.  And that is
  ; the behavior I want.
  ;
  ; When running this from a webserver, certain contitions (and especially
  ; core dumps) are nearly impossible to debug.  But by setting gf_debug to
  ; true and making a request, the process will hang, allowing me to run gdb
  ; against the running process.  Once hooked up, I set gf_debug to false
  ; and then continue with the execution.  Then, I can see where it crashes,
  ; single step, what have you, without having to set up a huge fake "web"
  ; environment.  It's something I rarely do, but when I need it, I need it.
  ;-------------------------------------------------------------------------*/

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
  struct tm *ptm;
  char       buffer[128];
  char      *p;
  
  gd.tst              = time(NULL);
  ptm                 = localtime(&gd.tst);
  gd.stmst            = *ptm;
  gd.updatetime.year  = ptm->tm_year + 1900;
  gd.updatetime.month = ptm->tm_mon + 1;
  gd.updatetime.day   = ptm->tm_mday;
  gd.updatetime.part  = 1;
  
  fp = fopen(".first","r");
  if (fp == NULL)
  {
    gd.begin = gd.updatetime;
    fp = fopen(".first","w");
    if (fp)
    {
      fprintf(
      	       fp,
      	       "%4d/%02d/%02d.%d\n",
      	       gd.begin.year,
      	       gd.begin.month,
      	       gd.begin.day,
      	       gd.begin.part
      	     );
      fclose(fp);
    }
    else
      syslog(LOG_ERR,".first: %s",strerror(errno));
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
    gd.now = gd.updatetime;
    fp = fopen(".last","w");
    if (fp)
    {
      fprintf(
               fp,
               "%4d/%02d/%02d.%d\n",
               gd.now.year,
               gd.now.month,
               gd.now.day,
               gd.now.part
             );
      fclose(fp);
    }
    else
      syslog(LOG_ERR,".last: %s",strerror(errno));
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

