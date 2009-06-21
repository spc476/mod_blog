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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>

#include <cgilib6/pair.h>
#include <cgilib6/nodelist.h>
#include <cgilib6/errors.h>
#include <cgilib6/util.h>
#include <cgilib6/htmltok.h>

#include "conversion.h"

/**********************************************************************/

struct nested_params
{
  char        *fname;
  FILE        *in;
  FILE        *out;
  HtmlToken    token;
  bool         p;
  bool         pre;
  bool         list;
  bool         blockquote;
};

/**********************************************************************/

static void	text_conversion_backend	(char *,FILE *,FILE *,bool);
static void	html_handle_tag		(struct nested_params *);
static void	check_for_uri		(struct nested_params *,const char *);
static void	entify_char		(char *,size_t,char *,char,const char *);
static void	html_handle_string	(struct nested_params *);
static void	html_handle_comment	(struct nested_params *);
static void	handle_backquote	(FILE *,FILE *);
static void	handle_quote		(FILE *,FILE *);
static void	handle_dash		(FILE *,FILE *);
static void	handle_period		(FILE *,FILE *);

/**********************************************************************/

void text_conversion(char *fname,FILE *in,FILE *out)
{
  assert(fname != NULL);
  assert(in    != NULL);
  assert(out   != NULL);

  text_conversion_backend(fname,in,out,true);
}

/**********************************************************************/

static void text_conversion_backend(char *fname,FILE *in,FILE *out,bool entities)
{
#if 0
  Stream  tmpout;
  char   *line;
  char   *newline;

  assert(fname != NULL);
  assert(in    != NULL);
  assert(out   != NULL);
  
  tmpout = StringStreamWrite();

  while(!StreamEOF(in))
  {
    line = LineSRead(in);

    if (empty_string(line))
    {
      newline = StringFromStream(tmpout);
      if (!empty_string(newline))
        LineSFormat(out,"$","<p>%a</p>\n",newline);
      free(newline);
      StreamFlush(tmpout);
    }
    else
    {
      Stream tmpin;
      Stream filter;
      
      tmpin = MemoryStreamRead(line,strlen(line));
  
      if (entities)
      {
        filter = EntityStreamRead(tmpin,ENTITY_NOAMP);
        buff_conversion(filter,tmpout,QUOTE_SMART);
        StreamFree(filter);
      }
      else
        buff_conversion(tmpin,tmpout,QUOTE_SMART);
  
      StreamWrite(tmpout,' ');
      StreamFree(tmpin);
    }
    
    free(line);
  }
  
  newline = StringFromStream(tmpout);
  if (!empty_string(newline))
    LineSFormat(out,"$","<p>%a</p>\n",newline);
  
  free(newline);
  StreamFree(tmpout);
#endif
}
  
/***********************************************************************/

void mixed_conversion(char *fname,FILE *in,FILE *out)
{
#if 0
  Stream  tmp;
  char   *line;

  assert(fname != NULL);
  assert(in    != NULL);
  assert(out   != NULL);
  
  tmp = StringStreamWrite();
  text_conversion_backend(fname,in,tmp,false);
  
  line = StringFromStream(tmp);
  StreamFree(tmp);
  
  tmp = MemoryStreamRead(line,strlen(line));
  html_conversion(fname,tmp,out);
  
  StreamFree(tmp);
  free(line);
#endif
}


/*************************************************************************/

void html_conversion(char *fname,FILE *in,FILE *out)
{
  struct nested_params local;
  int                  t;

  assert(fname != NULL);
  assert(in    != NULL);
  assert(out   != NULL);
    
  local.fname      = fname;
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

static void check_for_uri(struct nested_params *local,const char *attrib)
{
  char         newuri[BUFSIZ];
  struct pair *src;
  struct pair *np;
  
  assert(local  != NULL);
  assert(attrib != NULL);
  
  src = HtmlParseGetPair(local->token,attrib);
  if (src == NULL) return;

  entify_char(newuri,BUFSIZ,src->value,'&',"&amp;");
    
  np = PairCreate(attrib,newuri);
  NodeInsert(&src->node,&np->node);
  NodeRemove(&src->node);
  PairFree(src);
}

/*************************************************************************/

static void entify_char(char *d,size_t ds,char *s,char e,const char *entity)
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
  FILE *in;
  char *text = HtmlParseValue(local->token);
  
  assert(local != NULL);

  /*---------------------------------------------------
  ; I think is is why my tags sometime run together
  ;---------------------------------------------------*/

  /*if (empty_string(text)) return;*/	

  if (!local->pre)
  {
    in = fmemopen(text,strlen(text),"r");
    buff_conversion(in,local->out,QUOTE_DUMB);
    fclose(in);
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

void buff_conversion(FILE * in,FILE * out,int quotetype)
{
  int c;
  
  assert(in  != NULL);
  assert(out != NULL);

  /*----------------------------------------------------------------
  ; XXX
  ; how to handle using '"' for smart quotes, or regular
  ; quotes.  I'd rather not have to do logic, but anything else
  ; might require more coupling than I want.  Have to think
  ; on this.  Also, add in some logic to handle '&' in strings.
  ; look for something like &[^\s]+; and if so, then pass
  ; it through unchanged; if not, then convert '&' to "&amp;".
  ;
  ; Okay, use QUOTE_SMART and QUOTE_DUMB when I get back to this.
  ;---------------------------------------------------------------*/
    
  while(!feof(in))
  {
    c = fgetc(in);
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

static void handle_backquote(FILE *input,FILE *output)
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

static void handle_quote(FILE *input,FILE *output)
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

static void handle_dash(FILE *input,FILE *output)
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

static void handle_period(FILE *input,FILE *output)
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

