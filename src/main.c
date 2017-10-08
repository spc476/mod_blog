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

#include <stdlib.h>
#include <cgilib6/crashreport.h>
#include "globals.h"

/************************************************************************/

int main(int argc,char *argv[],char *envp[])
{
  /*-----------------------------------------------------------------------
  ; due to a crash for no apparent reason, I decided to log a bit more
  ; information when it does happen.  Here are the possible signals I want
  ; more information from when it happens.
  ;-----------------------------------------------------------------------*/
  
  crashreport_with(argc,argv,envp);
  
  /*---------------------
  ; Defined in ANSI C
  ;-----------------------*/
  
  crashreport(SIGABRT);
  crashreport(SIGFPE);
  crashreport(SIGILL);
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
    
  gd.cgi = CgiNew(NULL);
  
  if (gd.cgi != NULL)
  {
    switch(CgiMethod(gd.cgi))
    {
      case GET:  return main_cgi_get (gd.cgi);
      case POST: return main_cgi_post(gd.cgi);
      case HEAD: return main_cgi_head(gd.cgi);
      default:   return EXIT_FAILURE;
    }
  }
  
  return main_cli(argc,argv);
}

/***********************************************************************/
