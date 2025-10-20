/*********************************************************************
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
*********************************************************************/

#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <cgilib8/util.h>
#include <cgilib8/htmltok.h>

#include "conversion.h"
#include "blogutil.h"

/*********************************************************************/

static ssize_t fj_write(void *cookie,char const *buffer,size_t bytes)
{
  FILE *realout = cookie;
  size_t size   = bytes;
  
  assert(cookie != NULL);
  assert(buffer != NULL);
  
  while(size)
  {
    switch(*buffer)
    {
      case '"' : fputc('\\',realout); fputc(*buffer,realout); break;
      case '\\': fputc('\\',realout); fputc(*buffer,realout); break;
      case '\b': fputc('\\',realout); fputc('b',realout);     break;
      case '\f': fputc('\\',realout); fputc('f',realout);     break;
      case '\n': fputc('\\',realout); fputc('n',realout);     break;
      case '\r': fputc('\\',realout); fputc('r',realout);     break;
      case '\t': fputc('\\',realout); fputc('t',realout);     break;
      default:   fputc(*buffer,realout);                      break;
    }
    buffer++;
    size--;
  }
  
  return bytes;
}

/*********************************************************************/

FILE *fjson_encode_onwrite(FILE *out)
{
  assert(out != NULL);
  return fopencookie(out,"w",(cookie_io_functions_t) {
                               NULL,
                               fj_write,
                               NULL,
                               NULL
                             });
}

/*********************************************************************/
