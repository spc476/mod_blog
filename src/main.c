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

#include <cgilib7/crashreport.h>
#include "main.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  Cgi cgi;
  int rc;
  
  crashreport_args(argc,argv);
  crashreport_core();
  
  cgi = CgiNew();
  
  if (cgi != NULL)
  {
    switch(CgiMethod(cgi))
    {
      case HEAD:
      case GET:  rc = main_cgi_GET (cgi); break;
      case POST: rc = main_cgi_POST(cgi); break;
      case PUT:  rc = main_cgi_PUT (cgi); break;
      default:   rc = main_cgi_bad (cgi); break;
    }
    CgiFree(cgi);
  }
  else
    rc = main_cli(argc,argv);
    
  return rc;
}

/***********************************************************************/
