/***************************************************
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
*****************************************************/

#include <string.h>
#include <stdlib.h>

#include <cgilib6/util.h>

#include "conversion.h"
#include "frontend.h"

/****************************************************/

void request_free(Request *request)
{
  assert(request != NULL);
  
  free(request->origauthor);
  free(request->author);
  free(request->title);
  free(request->class);
  free(request->status);
  free(request->date);
  free(request->adtag);
  free(request->origbody);
  free(request->body);
}

/***********************************************************************/

#if 0
bool TO_email(char const *value,bool def)
{
  if (!emptynull_string(value))
  {
    if (strcmp(value,"no") == 0)
      return false;
    else if (strcmp(value,"yes") == 0)
      return true;
  }

  return def;
}
#endif

/***************************************************************************/
