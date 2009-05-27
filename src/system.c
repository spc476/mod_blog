/**************************************************************
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
******************************************************************/

#if !(defined(__unix__) || defined(__MACH__))
#  error This code is Unix specific.  You have been warned.
#endif

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>

#include <cgilib/pair.h>
#include <cgilib/ddt.h>
#include <cgilib/util.h>

/*******************************************************************/

static int limit_resource(int resource,struct pair *data)
{
  struct rlimit limit;
  int           rc;

  ddt(data != NULL);
  
  rc = getrlimit(resource,&limit);
  if (rc) return(rc);
  limit.rlim_max = strtoul(data->value,NULL,10);
  return(setrlimit(resource,&limit));
}

/*******************************************************************/

int SystemLimit(struct pair *data)
{

  ddt(data != NULL);

  if (strcmp(data->name,"_SYSTEM-CPU") == 0)
  {
#   ifdef RLIMIT_CPU
      return(limit_resource(RLIMIT_CPU,data));
#   else
      return(0);
#   endif
  }
  else if (strcmp(data->name,"_SYSTEM-MEM") == 0)
  {
#   ifdef RLIMIT_DATA
      return(limit_resource(RLIMIT_DATA,data));
#   elif defined (RLIMIT_VMEM)
      return(limit_resource(RLIMIT_VMEM,data));
#   else
      return(0);
#   endif
  }
  else if (strcmp(data->name,"_SYSTEM-CORE") == 0)
  {
#   ifdef RLIMIT_CORE
      return(limit_resource(RLIMIT_CORE,data));
#   else
      return(0);
#   endif
  }
  else if (strcmp(data->name,"_SYSTEM-LOCALE") == 0)
  {
    char *ol;
    
    ol = setlocale(LC_ALL,data->value);
    if (ol == NULL)
      return(1);
    else
      return(0);
  }
  else if (strcmp(data->name,"_SYSTEM-IGNORE") == 0)
  {
    struct sigaction act;
    struct sigaction oact;
    int              sig;

    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    up_string(data->value);
    if (strcmp(data->value,"SIGTERM") == 0)
      sig = SIGTERM;
    else
      return(0);

    return(sigaction(sig,&act,&oact));
  }
  else
    return(0);  
}

/**********************************************************************/

