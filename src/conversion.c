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

/**********************************************************************/

static void handle_backquote(FILE *restrict input,FILE *restrict output)
{
  int c;
  
  assert(input  != NULL);
  assert(output != NULL);
  
  if (feof(input))
  {
    fputc('`',output);
    return;
  }
  
  c = fgetc(input);
  if (c == '`')
    fputs("&#8220;",output);
  else
  {
    fputc('`',output);
    ungetc(c,input);
  }
}

/********************************************************************/

static void handle_quote(FILE *restrict input,FILE *restrict output)
{
  int c;
  
  assert(input  != NULL);
  assert(output != NULL);
  
  if (feof(input))
  {
    fputc('\'',output);
    return;
  }
  
  c = fgetc(input);
  if (c == '\'')
    fputs("&#8221;",output);
  else
  {
    fputc('\'',output);
    ungetc(c,input);
  }
}

/**********************************************************************/

static void handle_dash(FILE *restrict input,FILE *restrict output)
{
  int c;
  
  assert(input  != NULL);
  assert(output != NULL);
  
  if (feof(input))
  {
    fputc('-',output);
    return;
  }
  
  c = fgetc(input);
  if (c == '-')
  {
    c = fgetc(input);
    if (c == '-')
      fputs("&#8212;",output);
    else
    {
      fputs("&#8211;",output);
      ungetc(c,input);
    }
  }
  else
  {
    fputc('-',output);
    ungetc(c,input);
  }
}

/********************************************************************/

static void handle_period(FILE *restrict input,FILE *restrict output)
{
  int c;
  
  assert(input  != NULL);
  assert(output != NULL);
  
  if (feof(input))
  {
    fputc('.',output);
    return;
  }
  
  c = fgetc(input);
  if (c == '.')
  {
    c = fgetc(input);
    if (c == '.')
      fputs("&#8230;",output);
    else
    {
      fputs("&#8229;",output);
      ungetc(c,input);
    }
  }
  else
  {
    fputc('.',output);
    ungetc(c,input);
  }
}

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

void buff_conversion(FILE *restrict in,FILE *restrict out)
{
  /*----------------------------------------------------
  ; this is basically the macro substitution module.
  ;-----------------------------------------------------*/
  
  assert(in  != NULL);
  assert(out != NULL);
  
  /*------------------------------------------------------------------------
  ; XXX how to handle using '"' for smart quotes, or regular quotes.  I'd
  ; rather not have to do logic, but anything else might require more
  ; coupling than I want.  Have to think on this.  Also, add in some logic
  ; to handle '&' in strings.  look for something like &[^\s]+; and if so,
  ; then pass it through unchanged; if not, then convert '&' to "&amp;".
  ;------------------------------------------------------------------------*/
  
  while(!feof(in))
  {
    int c = fgetc(in);
    switch(c)
    {
      case EOF:  break;
      case '`':  handle_backquote(in,out); break;
      case '\'': handle_quote(in,out); break;
      case '-':  handle_dash(in,out); break;
      case '.':  handle_period(in,out); break;
      case '\0': assert(0);
      default:   fputc(c,out); break;
    }
  }
}

/*****************************************************************/

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
