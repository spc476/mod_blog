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

int main(int argc,char *argv[])
{
  extern char **environ;
  Cgi cgi;
  int rc;
  
  crashreport_with(argc,argv,environ);
  crashreport_core();
  
  cgi = CgiNew(NULL);
  
  if (cgi != NULL)
  {
    switch(CgiMethod(cgi))
    {
      case HEAD:
      case GET:  rc = main_cgi_get (cgi); break;
      case POST: rc = main_cgi_post(cgi); break;
      default:   rc = EXIT_FAILURE;       break;
    }
    CgiFree(cgi);
  }
  else
    rc = main_cli(argc,argv);
    
  return rc;
}

/***********************************************************************/
