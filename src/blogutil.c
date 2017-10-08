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
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "blogutil.h"

/*******************************************************************/

String *tag_split(size_t *const pnum,char const *tag)
{
  size_t      num  = 0;
  size_t      max  = 0;
  String     *pool = NULL;
  char const *p;
  
  assert(pnum != NULL);
  assert(tag  != NULL);
  
  while(*tag)
  {
    if (num == max)
    {
      max += 1024;
      pool = realloc(pool,max * sizeof(String));
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
  return pool;
}

/*********************************************************************/

char *fromstring(String const src)
{
  char *dest;
  
  dest = malloc(src.s + 1);
  memcpy(dest,src.d,src.s);
  dest[src.s] = '\0';
  return dest;
}

/*********************************************************************/

size_t fcopy(FILE *const restrict out,FILE *const restrict in)
{
  char   buffer[BUFSIZ];
  size_t inbytes;
  size_t outbytes;
  
  assert(out != NULL);
  assert(in  != NULL);
  
  outbytes = 0;
  
  do
  {
    inbytes   = fread(buffer,1,BUFSIZ,in);
    outbytes += fwrite(buffer,1,inbytes,out);
  } while (inbytes > 0);
  
  return outbytes;
}

/*********************************************************************/
