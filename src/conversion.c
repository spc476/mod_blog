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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <cgilib6/util.h>
#include <cgilib6/htmltok.h>

#include "conversion.h"
#include "frontend.h"
#include "blogutil.h"

/**********************************************************************/

struct nested_params
{
  FILE        *in;
  FILE        *out;
  HtmlToken    token;
  bool         p;
  bool         pre;
  bool         list;
  bool         blockquote;
};

/**********************************************************************/

static void text_conversion_backend (FILE *restrict,FILE *restrict);
static void html_handle_tag         (struct nested_params *);
static void check_for_uri           (struct nested_params *,char const *);
static void entify_char             (char *,size_t,char *,char,char const *);
static void html_handle_string      (struct nested_params *);
static void html_handle_comment     (struct nested_params *);
static void handle_backquote        (FILE *restrict,FILE *restrict);
static void handle_quote            (FILE *restrict,FILE *restrict);
static void handle_dash             (FILE *restrict,FILE *restrict);
static void handle_period           (FILE *restrict,FILE *restrict);

/**********************************************************************/

conversion__f TO_conversion(char const *name)
{
  assert(name != NULL);
  
  if (strcmp(name,"text") == 0)
    return text_conversion;
  else if (strcmp(name,"mixed") == 0)
    return mixed_conversion;
  else if (strcmp(name,"html") == 0)
    return html_conversion;
  else if (strcmp(name,"none") == 0)
    return no_conversion;
  else
  {
    syslog(LOG_WARNING,"conversion '%s' unsupported",name);
    return no_conversion;
  }
}

/**********************************************************************/

void no_conversion(FILE *restrict in,FILE *restrict out)
{
  assert(in  != NULL);
  assert(out != NULL);
  
  fcopy(out,in);
}

/**********************************************************************/

void text_conversion(FILE *restrict in,FILE *restrict out)
{
  FILE *entin;
  
  assert(in    != NULL);
  assert(out   != NULL);
  
  entin = fentity_encode_onread(in);
  text_conversion_backend(entin,out);
  fclose(entin);
}

/**********************************************************************/

static void text_conversion_backend(FILE *restrict in,FILE *restrict out)
{
  FILE   *tmpout;
  char   *buffer;
  char   *line;
  size_t  bufsize;
  size_t  linesize;
  
  assert(in  != NULL);
  assert(out != NULL);
  
  line     = NULL;
  buffer   = NULL;
  bufsize  = 0;
  linesize = 0;
  tmpout   = open_memstream(&buffer,&bufsize);
  
  while(!feof(in))
  {
    ssize_t  bytes;
    
    bytes = getline(&line,&linesize,in);
    if (bytes <= 0) break;
    line[--bytes] = '\0';
    
    if (empty_string(line))
    {
      fclose(tmpout);
      
      if (!empty_string(buffer))
        fprintf(out,"<p>%s</p>\n",buffer);
        
      free(buffer);
      
      buffer  = NULL;
      bufsize = 0;
      tmpout  = open_memstream(&buffer,&bufsize);
    }
    else
    {
      FILE *tmpin;
      
      tmpin = fmemopen(line,bytes,"r");
      buff_conversion(tmpin,tmpout);
      fclose(tmpin);
      fputc(' ',tmpout);
    }
  }
  
  fclose(tmpout);
  if (!empty_string(buffer))
    fprintf(out,"<p>%s</p>\n",buffer);
    
  free(line);
  free(buffer);
}

/***********************************************************************/

void mixed_conversion(FILE *restrict in,FILE *restrict out)
{
  FILE *tmpfp;
  char *text;
  size_t size;
  
  assert(in  != NULL);
  assert(out != NULL);
  
  tmpfp = open_memstream(&text,&size);
  text_conversion_backend(in,tmpfp);
  fclose(tmpfp);
  
  tmpfp = fmemopen(text,size,"r");
  html_conversion(tmpfp,out);
  fclose(tmpfp);
  
  free(text);
}

/*************************************************************************/

void html_conversion(FILE *restrict in,FILE *restrict out)
{
  struct nested_params local;
  int                  t;
  
  assert(in  != NULL);
  assert(out != NULL);
  
  local.in         = in;
  local.out        = out;
  local.p          = false;
  local.pre        = false;
  local.blockquote = false;
  local.list       = false;
  
  local.token = HtmlParseNew(local.in);
  
  while((t = HtmlParseNext(local.token)) != T_EOF)
  {
    if (t == T_TAG)
      html_handle_tag(&local);
    else if (t == T_COMMENT)
      html_handle_comment(&local);
    else if (t == T_STRING)
      html_handle_string(&local);
  }
  
  HtmlParseFree(local.token);
}

/**********************************************************************/

static void html_handle_tag(struct nested_params *local)
{
  assert(local != NULL);
  
  /*---------------------------------------------------------
  ; tags that change a state
  ;----------------------------------------------------------*/
  
  if (strcmp(HtmlParseValue(local->token),"P") == 0)
    local->p = true;
  else if (strcmp(HtmlParseValue(local->token),"/P") == 0)
    local->p = false;
  else if (strcmp(HtmlParseValue(local->token),"PRE") == 0)
  {
    local->pre = true;
    local->p   = false;
  }
  else if (strcmp(HtmlParseValue(local->token),"/PRE") == 0)
    local->pre = false;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"TT") == 0)
    || (strcmp(HtmlParseValue(local->token),"CODE") == 0)
    || (strcmp(HtmlParseValue(local->token),"SAMP") == 0)
    || (strcmp(HtmlParseValue(local->token),"KBD")  == 0)
    || (strcmp(HtmlParseValue(local->token),"VAR")  == 0)
  )
    local->pre = true;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"/VAR") == 0)
    || (strcmp(HtmlParseValue(local->token),"/KBD")  == 0)
    || (strcmp(HtmlParseValue(local->token),"/SAMP") == 0)
    || (strcmp(HtmlParseValue(local->token),"/CODE") == 0)
    || (strcmp(HtmlParseValue(local->token),"/TT")   == 0)
  )
    local->pre = false;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"UL") == 0)
    || (strcmp(HtmlParseValue(local->token),"OL") == 0)
  )
    local->list = true;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"/UL") == 0)
    || (strcmp(HtmlParseValue(local->token),"/OL") == 0)
  )
    local->list = false;
    
  /*--------------------------------------------------------------
  ; tags that have URIs that need to be checked for & and fixed.
  ;--------------------------------------------------------------*/
  
  else if (strcmp(HtmlParseValue(local->token),"A") == 0)
    check_for_uri(local,"HREF");
  else if (strcmp(HtmlParseValue(local->token),"BLOCKQUOTE") == 0)
  {
    local->p          = false;
    local->blockquote = true;
    check_for_uri(local,"CITE");
  }
  else if (strcmp(HtmlParseValue(local->token),"/BLOCKQUOTE") == 0)
    local->blockquote = false;
  else if (strcmp(HtmlParseValue(local->token),"AREA") == 0)
    check_for_uri(local,"HREF");
  else if (strcmp(HtmlParseValue(local->token),"LINK") == 0)
    check_for_uri(local,"HREF");
  else if (strcmp(HtmlParseValue(local->token),"BASE") == 0)
    check_for_uri(local,"HREF");
  else if (strcmp(HtmlParseValue(local->token),"IMG") == 0)
  {
    check_for_uri(local,"SRC");
    check_for_uri(local,"LONGDESC");
    check_for_uri(local,"USEMAP");
  }
  else if (strcmp(HtmlParseValue(local->token),"OBJECT") == 0)
  {
    check_for_uri(local,"CLASSID");
    check_for_uri(local,"CODEBASE");
    check_for_uri(local,"DATA");
    check_for_uri(local,"ARCHIVE");
    check_for_uri(local,"USEMAP");
  }
  else if (strcmp(HtmlParseValue(local->token),"Q") == 0)
    check_for_uri(local,"CITE");
  else if (strcmp(HtmlParseValue(local->token),"INS") == 0)
    check_for_uri(local,"CITE");
  else if (strcmp(HtmlParseValue(local->token),"DEL") == 0)
    check_for_uri(local,"CITE");
  else if (strcmp(HtmlParseValue(local->token),"FORM") == 0)
    check_for_uri(local,"ACTION");
  else if (strcmp(HtmlParseValue(local->token),"INPUT") == 0)
  {
    check_for_uri(local,"SRC");
    check_for_uri(local,"USEMAP");
  }
  else if (strcmp(HtmlParseValue(local->token),"HEAD") == 0)
    check_for_uri(local,"PROFILE");
  else if (strcmp(HtmlParseValue(local->token),"SCRIPT") == 0)
  {
    check_for_uri(local,"SRC");
    check_for_uri(local,"FOR");
  }
  else
  {
    if (!local->blockquote)
      local->p = false;
  }
  
  HtmlParsePrintTag(local->token,local->out);
}

/************************************************************************/

static void check_for_uri(struct nested_params *local,char const *attrib)
{
  char         newuri[BUFSIZ];
  struct pair *src;
  struct pair *np;
  
  assert(local  != NULL);
  assert(attrib != NULL);
  
  src = HtmlParseGetPair(local->token,attrib);
  if (src == NULL) return;
  
  entify_char(newuri,sizeof(newuri),src->value,'&',"&amp;");
  
  np = PairCreate(attrib,newuri);
  NodeInsert(&src->node,&np->node);
  NodeRemove(&src->node);
  PairFree(src);
}

/*************************************************************************/

static void entify_char(char *d,size_t ds,char *s,char e,char const *entity)
{
  size_t se;
  
  assert(d      != NULL);
  assert(ds     >  0);
  assert(s      != NULL);
  assert(e      != '\0');
  assert(entity != NULL);
  
  se = strlen(entity);
  
  for ( ; (*s) && (ds > 0) ; )
  {
    if (*s == e)
    {
      if (ds < se)
      {
        *d = '\0';
        return;
      }
      memcpy(d,entity,se);
      d  += se;
      ds -= se;
      s++;
    }
    else
    {
      *d++ = *s++;
      ds--;
    }
  }
  
  *d = '\0';
}

/**************************************************************************/

static void html_handle_string(struct nested_params *local)
{
  assert(local != NULL);
  
  char *text = HtmlParseValue(local->token);
  
  if (!local->pre)
  {
    FILE *in = fmemopen(text,strlen(text),"r");
    if (in != NULL)
    {
      buff_conversion(in,local->out);
      fclose(in);
    }
  }
  else
    fputs(text,local->out);
}

/************************************************************************/

static void html_handle_comment(struct nested_params *local)
{
  assert(local != NULL);
  
  fprintf(local->out,"<!%s>",HtmlParseValue(local->token));
}

/**************************************************************************/

void buff_conversion(FILE *restrict in,FILE *restrict out)
{
  /*----------------------------------------------------
  ; this is basically the macro substitution module.
  ;-----------------------------------------------------*/
  
  assert(in  != NULL);
  assert(out != NULL);
  
  /*------------------------------------------------------------------------
  ; XXX
  ; how to handle using '"' for smart quotes, or regular quotes.  I'd rather
  ; not have to do logic, but anything else might require more coupling than
  ; I want.  Have to think on this.  Also, add in some logic to handle '&'
  ; in strings.  look for something like &[^\s]+; and if so, then pass it
  ; through unchanged; if not, then convert '&' to "&amp;".
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
