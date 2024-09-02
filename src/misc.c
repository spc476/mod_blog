/*********************************************************************
*
* Copyright 2024 by Sean Conner.  All Rights Reserved.
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
**********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "frontend.h"
#include "conversion.h"

/************************************************************************/

char *safe_strdup(char const *orig)
{
  return orig != NULL ? strdup(orig) : strdup("");
}

/************************************************************************/

char *safe_getenv(char const *env)
{
  assert(env != NULL);
  
  char *e = getenv(env);
  if (e == NULL)
    e = (char *)"";
  return e;
}

/************************************************************************/

Request *request_init(Request *request)
{
  assert(request != NULL);
  
  memset(request,0,sizeof(Request));
  request->origauthor = NULL;
  request->author     = NULL;
  request->title      = NULL;
  request->class      = NULL;
  request->status     = NULL;
  request->date       = NULL;
  request->adtag      = NULL;
  request->origbody   = NULL;
  request->body       = NULL;
  request->reqtumbler = NULL;
  request->conversion = no_conversion;
  return request;
}

/************************************************************************/

void request_free(Request *request)
{
  assert(request != NULL);
  
  /*-----------------------------------------------------------------------
  ; Not all fields need to be freed.  I need to figure out why.  Is my code
  ; now legacy?  That's ...  scary.
  ;------------------------------------------------------------------------*/
  
  free(request->origauthor);
  free(request->date);
  free(request->adtag);
  free(request->origbody);
}

/************************************************************************/
