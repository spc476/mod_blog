/****************************************************************
*
* Copyright 2006 by Sean Conner.  All Rights Reserved.
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
*****************************************************************/

#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>

#include "blogutil.h"

/*******************************************************************/

String *tag_split(size_t *pnum,const char *tag)
{
  size_t      num  = 0;
  size_t      max  = 0;
  String     *pool = NULL;
  const char *p;
  
  ddt(pnum != NULL);
  ddt(tag  != NULL);
  
  while(*tag)
  {
    if (num == max)
    {
      max += 1024;
      pool = MemResize(pool,max * sizeof(String));
    }
    
    for (p = tag ; (*p) && (*p != ',') ; p++)
      ;
    if (p == tag) break;

    pool[num].d   = tag;
    pool[num++].s = p - tag;
    
    if (*p == '\0')
      break;
    for (p++ ; (*p) && isspace(*p) ; p++)
      ;
    tag = p;
  }
  
  *pnum = num;
  return(pool);
}

/*********************************************************************/

char *fromstring(String src)
{
  char *dest;
  
  dest = MemAlloc(src.s + 1);
  memcpy(dest,src.d,src.s);
  dest[src.s] = '\0';
  return (dest);
}

/*********************************************************************/

