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

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/stream.h>
#include <cgilib/pair.h>
#include <cgilib/nodelist.h>
#include <cgilib/errors.h>
#include <cgilib/types.h>
#include <cgilib/util.h>
#include <cgilib/htmltok.h>

#include "conversion.h"

/**********************************************************************/

struct nested_params
{
  char        *fname;
  Stream       in;
  Stream       out;
  HtmlToken    token;
  int          p;
  int          pre;
  int          list;
  int          blockquote;
};

/**********************************************************************/

static void	text_conversion_backend	(char *,Stream,Stream,int);
static void	html_handle_tag		(struct nested_params *);
static void	check_for_uri		(struct nested_params *,const char *);
static void	entify_char		(char *,size_t,char *,char,const char *);
static void	html_handle_string	(struct nested_params *);
static void	html_handle_comment	(struct nested_params *);
static void	handle_backquote	(Stream,Stream);
static void	handle_quote		(Stream,Stream);
static void	handle_dash		(Stream,Stream);
static void	handle_period		(Stream,Stream);

/**********************************************************************/

void text_conversion(char *fname,Stream in,Stream out)
{
  ddt(fname != NULL);
  ddt(in    != NULL);
  ddt(out   != NULL);

  text_conversion_backend(fname,in,out,TRUE);
}

/**********************************************************************/

static void text_conversion_backend(char *fname,Stream in,Stream out,int entities)
{
  Stream  tmpout;
  char   *line;
  char   *newline;

  ddt(fname != NULL);
  ddt(in    != NULL);
  ddt(out   != NULL);
  
  tmpout = StringStreamWrite();

  while(!StreamEOF(in))
  {
    line = LineSRead(in);

    if (empty_string(line))
    {
      newline = StringFromStream(tmpout);
      if (!empty_string(newline))
        LineSFormat(out,"$","<p>%a</p>\n",newline);
      MemFree(newline);
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
    
    MemFree(line);
  }
  
  newline = StringFromStream(tmpout);
  if (!empty_string(newline))
    LineSFormat(out,"$","<p>%a</p>\n",newline);
  
  MemFree(newline);
  StreamFree(tmpout);
}
  
/***********************************************************************/

void mixed_conversion(char *fname,Stream in,Stream out)
{
  Stream  tmp;
  char   *line;

  ddt(fname != NULL);
  ddt(in    != NULL);
  ddt(out   != NULL);
  
  tmp = StringStreamWrite();
  text_conversion_backend(fname,in,tmp,FALSE);
  
  line = StringFromStream(tmp);
  StreamFree(tmp);
  
  tmp = MemoryStreamRead(line,strlen(line));
  html_conversion(fname,tmp,out);
  
  StreamFree(tmp);
  MemFree(line);
}


/*************************************************************************/

void html_conversion(char *fname,Stream in,Stream out)
{
  struct nested_params local;
  int                  t;

  ddt(fname != NULL);
  ddt(in    != NULL);
  ddt(out   != NULL);
    
  local.fname      = fname;
  local.in         = in;
  local.out        = out;
  local.p          = FALSE;
  local.pre        = FALSE;
  local.blockquote = FALSE;
  local.list       = FALSE;
  
  HtmlParseNew(&local.token,local.in);
  
  while((t = HtmlParseNext(local.token)) != T_EOF)
  {
    if (t == T_TAG)
      html_handle_tag(&local);
    else if (t == T_COMMENT)
      html_handle_comment(&local);
    else if (t == T_STRING)
      html_handle_string(&local);
  }
  
  HtmlParseFree(&local.token);
}

/**********************************************************************/

static void html_handle_tag(struct nested_params *local)
{
  ddt(local != NULL);
  
  /*---------------------------------------------------------
  ; tags that change a state
  ;----------------------------------------------------------*/
  
  if (strcmp(HtmlParseValue(local->token),"P") == 0)
    local->p = TRUE;
  else if (strcmp(HtmlParseValue(local->token),"/P") == 0)
    local->p = FALSE;
  else if (strcmp(HtmlParseValue(local->token),"PRE") == 0)
  {
    local->pre = TRUE;
    local->p   = FALSE;
  }
  else if (strcmp(HtmlParseValue(local->token),"/PRE") == 0)
    local->pre = FALSE;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"TT") == 0)
    || (strcmp(HtmlParseValue(local->token),"CODE") == 0)
    || (strcmp(HtmlParseValue(local->token),"SAMP") == 0)
    || (strcmp(HtmlParseValue(local->token),"KBD")  == 0)
    || (strcmp(HtmlParseValue(local->token),"VAR")  == 0)
  )
    local->pre = TRUE;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"/VAR") == 0)
    || (strcmp(HtmlParseValue(local->token),"/KBD")  == 0)
    || (strcmp(HtmlParseValue(local->token),"/SAMP") == 0)
    || (strcmp(HtmlParseValue(local->token),"/CODE") == 0)
    || (strcmp(HtmlParseValue(local->token),"/TT")   == 0)
  )
    local->pre = FALSE;
  else if
  (
    (strcmp(HtmlParseValue(local->token),"UL") == 0)
    || (strcmp(HtmlParseValue(local->token),"OL") == 0)
  )
    local->list = TRUE;
  else if 
  (
    (strcmp(HtmlParseValue(local->token),"/UL") == 0)
    || (strcmp(HtmlParseValue(local->token),"/OL") == 0)
  )
    local->list = FALSE;

  /*--------------------------------------------------------------
  ; tags that have URIs that need to be checked for & and fixed.
  ;--------------------------------------------------------------*/
  
  else if (strcmp(HtmlParseValue(local->token),"A") == 0)
    check_for_uri(local,"HREF");
  else if (strcmp(HtmlParseValue(local->token),"BLOCKQUOTE") == 0)
  {
    local->p          = FALSE;
    local->blockquote = TRUE;
    check_for_uri(local,"CITE");
  }
  else if (strcmp(HtmlParseValue(local->token),"/BLOCKQUOTE") == 0)
    local->blockquote = FALSE;
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
      local->p = FALSE;
  }
  
  HtmlParsePrintTag(local->token,local->out);
}

/************************************************************************/

static void check_for_uri(struct nested_params *local,const char *attrib)
{
  char         newuri[BUFSIZ];
  struct pair *src;
  struct pair *np;
  
  ddt(local  != NULL);
  ddt(attrib != NULL);
  
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
  
  ddt(d      != NULL);
  ddt(ds     >  0);
  ddt(s      != NULL);
  ddt(e      != '\0');
  ddt(entity != NULL);
  
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
  Stream  in;
  char   *text = HtmlParseValue(local->token);
  
  ddt(local != NULL);

  if (empty_string(text)) return;

  if (!local->pre)
  {
    in = MemoryStreamRead(text,strlen(text));
    buff_conversion(in,local->out,QUOTE_DUMB);
    StreamFree(in);
  }
  else
    LineS(local->out,text);
}

/************************************************************************/

static void html_handle_comment(struct nested_params *local)
{
  ddt(local != NULL);
  
  LineSFormat(local->out,"$","<!%a>",HtmlParseValue(local->token));
}

/**************************************************************************/

void buff_conversion(Stream in,Stream out,int quotetype)
{
  char   c;
  
  ddt(in  != NULL);
  ddt(out != NULL);

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
    
  while(!StreamEOF(in))
  {
    c = StreamRead(in);
    switch(c)
    {
      case IEOF: break;
      case '`':  handle_backquote(in,out); break;
      case '\'': handle_quote(in,out); break;
      case '-':  handle_dash(in,out); break;
      case '.':  handle_period(in,out); break;
      case '\0': ddt(0);
      default:   StreamWrite(out,c); break;
    }
  }
}

/*****************************************************************/

static void handle_backquote(Stream input,Stream output)
{
  char   c;

  ddt(input  != NULL);
  ddt(output != NULL);  

  if (StreamEOF(input))
  {
    StreamWrite(output,'`');
    return;
  }

  c = StreamRead(input);
  if (c == '`')
    LineS(output,"&#8220;");
  else
  {
    StreamWrite(output,'`');
    StreamUnRead(input,c);
  }
}

/********************************************************************/

static void handle_quote(Stream input,Stream output)
{
  char   c;
  
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (StreamEOF(input))
  {
    StreamWrite(output,'\'');
    return;
  }

  c = StreamRead(input);
  if (c == '\'')
    LineS(output,"&#8221;");
  else
  {
    StreamWrite(output,'\'');
    StreamUnRead(input,c);
  }
}

/**********************************************************************/

static void handle_dash(Stream input,Stream output)
{
  char   c;
    
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (StreamEOF(input))
  {
    StreamWrite(output,'-');
    return;
  }

  c = StreamRead(input);
  if (c == '-')
  {
    c = StreamRead(input);
    if (c == '-')
      LineS(output,"&#8212;");
    else
    {
      LineS(output,"&#8211;");
      StreamUnRead(input,c);
    }
  }
  else
  {
    StreamWrite(output,'-');
    StreamUnRead(input,c);
  }
}

/********************************************************************/

static void handle_period(Stream input,Stream output)
{
  char   c;
  
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (StreamEOF(input))
  {
    StreamWrite(output,'.');
    return;
  }

  c = StreamRead(input);
  if (c == '.')
  {
    c = StreamRead(input);
    if (c == '.')
      LineS(output,"&#8230;");
    else
    {
      LineS(output,"&#8229;");
      StreamUnRead(input,c);
    }
  }
  else
  {
    StreamWrite(output,'.');
    StreamUnRead(input,c);
  }
}

/*********************************************************************/

