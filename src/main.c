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
  crashreport_with(argc,argv,envp);
  crashreport_core();
  
  gd.cgi = CgiNew(NULL);
  
  if (gd.cgi != NULL)
  {
    switch(CgiMethod(gd.cgi))
    {
      case GET:  return main_cgi_get (gd.cgi);
      case POST: return main_cgi_post(gd.cgi);
      case HEAD: return main_cgi_get (gd.cgi);
      default:   return EXIT_FAILURE;
    }
  }
  
  return main_cli(argc,argv);
}

/***********************************************************************/
