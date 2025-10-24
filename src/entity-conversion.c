/*********************************************************************
*
* Copyright 2009 by Sean Conner.  All Rights Reserved.
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
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/***********************************************************************/

static bool char_entity(char const **tag,size_t *ps,int c)
{
  assert(tag != NULL);
  assert(ps  != NULL);
  
  switch(c)
  {
    case '<': *tag = "&lt;";   *ps = 4; return true;
    case '>': *tag = "&gt;";   *ps = 4; return true;
    case '&': *tag = "&amp;";  *ps = 5; return true;
    case '"': *tag = "&quot;"; *ps = 6; return true;
    default:                            return false;
  }
}

/**********************************************************************/

static ssize_t few_write(void *cookie,char const *buffer,size_t bytes)
{
  FILE       *realout = cookie;
  size_t      size    = bytes;
  char const *replace;
  size_t      repsize;
  
  assert(buffer != NULL);
  assert(cookie != NULL);
  
  while(bytes)
  {
    if (char_entity(&replace,&repsize,*buffer))
      fputs(replace,realout);
    else
      fputc(*buffer,realout);
      
    buffer++;
    bytes--;
  }
  
  return size;
}

/*******************************************************************/

FILE *fentity_encode_onwrite(FILE *out)
{
  assert(out != NULL);
  return fopencookie(out,"w",(cookie_io_functions_t)
                                {
                                  NULL,
                                  few_write,
                                  NULL,
                                  NULL
                                });
}

/******************************************************************/
