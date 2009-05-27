
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

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/pair.h>
#include <cgi/nodelist.h>
#include <cgi/errors.h>
#include <cgi/util.h>
#include <cgi/htmltok.h>

#include "conversion.h"

/**********************************************************************/

struct nested_params
{
  char        *fname;
  Buffer       in;
  Buffer       out;
  Buffer       linput;
  HtmlToken    token;
  int          p;
  int          pre;
  int          list;
  int          blockquote;
};

/**********************************************************************/

static void	text_conversion_backend	(char *,Buffer,Buffer,int);
static void	html_handle_tag		(struct nested_params *);
static void	check_for_uri		(struct nested_params *,const char *);
static void	entify_char		(char *,size_t,char *,char,const char *);
static void	print_tag		(struct nested_params *);
static void	html_handle_string	(struct nested_params *);
static void	html_handle_comment	(struct nested_params *);
static void	handle_backquote	(Buffer,Buffer);
static void	handle_quote		(Buffer,Buffer);
static void	handle_dash		(Buffer,Buffer);
static void	handle_period		(Buffer,Buffer);

/**********************************************************************/

void text_conversion(char *fname,Buffer in,Buffer output)
{
  text_conversion_backend(fname,in,output,TRUE);
}

/**********************************************************************/

static void text_conversion_backend(char *fname,Buffer in,Buffer output,int entities)
{
  Buffer linebuf;
  Buffer tmp;
  char   buffer[BUFSIZ];
  size_t size;
  int    rc;
  
  ddt(fname  != NULL);
  ddt(in     != NULL);
  ddt(output != NULL);

  rc = DynamicBuffer(&tmp);
  if (rc != ERR_OKAY)
    return;
      
  rc = LineBuffer(&linebuf,in);
  if (rc != ERR_OKAY)
  {
    BufferFree(&tmp);
    return;
  }
  
  while(!BufferEOF(linebuf))
  {
    size = BUFSIZ;
    LineRead(linebuf,buffer,&size);
    
    ddtlog(ddtstream,"$","read: %a",buffer);
    
    if ((size == 0) || (empty_string(buffer)))
    {
      size_t wsize;
      long   seek;
      
      if (BufferSize(tmp) == 0) continue;

      seek = 0;
      BufferSeek(tmp,&seek,SEEK_START);
      
      wsize = 3;
      BufferWrite(output,"<P>",&wsize);
      
      ddtlog(ddtstream,"","about to copy line");
      BufferCopy(output,tmp);

      wsize = 5;
      BufferWrite(output,"</P>\n",&wsize);
      
      BufferFree(&tmp);
      DynamicBuffer(&tmp);
    }
    else
    {
      Buffer intmp;
      Buffer lintmp;

      /*--------------------------------------------------------
      ; XXX
      ; Okay, for smart quotes, we may need to keep some state
      ; because a starting quote may be on one line, and the 
      ; ending quote on another line, so we'll need to keep track
      ; of some state across lines.  Have to think about this ...
      ;-------------------------------------------------------------*/
            
      MemoryBuffer(&intmp,buffer,size);
      BufferIOCtl(intmp,CM_SETSIZE,size);
      LineBuffer(&lintmp,intmp);
      if (entities)
      {
	Buffer filter;

	EntityOutBuffer(&filter,tmp);
	BufferIOCtl(filter,CE_NOAMP);
	buff_conversion(lintmp,filter,QUOTE_SMART);
	size = 1;
	BufferWrite(filter," ",&size);
	BufferFree(&filter);
      }
      else
      {
	buff_conversion(lintmp,tmp,QUOTE_SMART);
	size = 1;
	BufferWrite(tmp," ",&size);
      }
      BufferFree(&lintmp);
      BufferFree(&intmp);
    }
  }

  if (BufferSize(tmp))
  {
    size_t wsize;
    long   seek;

    seek = 0;
    BufferSeek(tmp,&seek,SEEK_START);
    wsize = 3;
    BufferWrite(output,"<P>",&wsize);
    BufferCopy(output,tmp);
    wsize = 5;
    BufferWrite(output,"</P>\n",&wsize);
  }
  
  BufferFree(&linebuf);
  BufferFree(&tmp);
}

/***********************************************************************/

void mixed_conversion(char *fname,Buffer in,Buffer out)
{
  Buffer tmp;
  long   loc;
  
  ddt(fname != NULL);
  ddt(in    != NULL);
  ddt(out   != NULL);
  
  DynamicBuffer(&tmp);
  text_conversion_backend(fname,in,tmp,FALSE);
  loc = 0;
  BufferSeek(tmp,&loc,SEEK_START);
  html_conversion(fname,tmp,out);
}

/*************************************************************************/

void html_conversion(char *fname,Buffer in,Buffer out)
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
  
  LineBuffer(&local.linput,in);
  HtmlParseNew(&local.token,local.linput);
  
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
  BufferFree(&local.linput);
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
      
  print_tag(local);
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
      
static void print_tag(struct nested_params *local)
{
  struct pair *pp;
  char         tvalue[BUFSIZ];
  
  ddt(local != NULL);
  
  BufferFormatWrite(local->out,"$","<%a",HtmlParseValue(local->token));
  for (
        pp = HtmlParseFirstOption(local->token);
        NodeValid(&pp->node);
        pp = (struct pair *)NodeNext(&pp->node)
      )
  {
    char *sq;
    char *dq;
    char  q;
    char *value = pp->value;
    
    sq = strchr(value,'\'');
    dq = strchr(value,'"');
    
    if (dq == NULL) 
      q = '"';
    else if ((sq == NULL) && (dq != NULL))
      q = '\'';
    else
    {
      entify_char(tvalue,BUFSIZ,value,'"',"&quot;");
      value = tvalue;
      q     = '"';
    }
    
    if ((pp->name) && (!empty_string(pp->name)))
      BufferFormatWrite(local->out,"$"," %a",pp->name);
    if ((pp->value) && (!empty_string(pp->value)))
      BufferFormatWrite(local->out,"c $","=%a%b%a",q,pp->value);
    else
      BufferFormatWrite(local->out,"c","=%a%a",q);
  }
  
  {
    char c   = '>';
    size_t s = 1;
    
    BufferWrite(local->out,&c,&s);
  }
}

/**************************************************************************/

static void html_handle_string(struct nested_params *local)
{
  char *text = HtmlParseValue(local->token);
  
  ddt(local != NULL);
  
  if (*text == '\0') return;

  if (!local->pre)
  {
    Buffer tin;
    Buffer tlin;

    MemoryBuffer(&tin,text,strlen(text));
    BufferIOCtl(tin,CM_SETSIZE,strlen(text));
    LineBuffer(&tlin,tin);
    buff_conversion(tlin,local->out,QUOTE_DUMB);
    BufferFree(&tlin);
    BufferFree(&tin);
  }
  else
  {
    BufferFormatWrite(local->out,"$","%a",text);
  }
}

/************************************************************************/

static void html_handle_comment(struct nested_params *local)
{
  char *text = HtmlParseValue(local->token);
  
  ddt(local != NULL);
  
  BufferFormatWrite(local->out,"$","<!%a>",text);
}

/**************************************************************************/

void buff_conversion(Buffer in,Buffer out,int quotetype)
{
  char   c;
  size_t s;
  
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
    
  while(!BufferEOF(in))
  {
    LineReadC(in,&c);
    switch(c)
    {
      case '`':  handle_backquote(in,out); break;
      case '\'': handle_quote(in,out); break;
      case '-':  handle_dash(in,out); break;
      case '.':  handle_period(in,out); break;
      case '\0': ddt(0);
      default:
           s = 1;
           BufferWrite(out,&c,&s);
           break;
    }
  }
}

/*****************************************************************/

static void handle_backquote(Buffer input,Buffer output)
{
  char   c;
  char   co = '`';
  size_t s;

  ddt(input  != NULL);
  ddt(output != NULL);  

  if (LineEOF(input))
  {
    s = 1;
    BufferWrite(output,&co,&s);
    return;
  }

  LineReadC(input,&c);
  if (c == '`')
    BufferFormatWrite(output,"","&#8220;");
  else
  {
    s = 1;
    BufferWrite(output,&co,&s);
    LineUnReadC(input,c);
  }
}

/********************************************************************/

static void handle_quote(Buffer input,Buffer output)
{
  char   c;
  char   co = '\'';
  size_t s;
  
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (LineEOF(input))
  {
    s = 1;
    BufferWrite(output,&co,&s);
    return;
  }

  LineReadC(input,&c);
  if (c == '\'')
    BufferFormatWrite(output,"","&#8221;");
  else
  {
    s = 1;
    BufferWrite(output,&co,&s);
    LineUnReadC(input,c);
  }
}

/**********************************************************************/

static void handle_dash(Buffer input,Buffer output)
{
  char   c;
  char   co = '-';
  size_t s;
    
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (LineEOF(input))
  {
    s = 1;
    BufferWrite(output,&co,&s);
    return;
  }

  LineReadC(input,&c);
  if (c == '-')
  {
    LineReadC(input,&c);
    if (c == '-')
      BufferFormatWrite(output,"","&#8212;");
    else
    {
      BufferFormatWrite(output,"","&#8211;");
      LineUnReadC(input,c);
    }
  }
  else
  {
    s = 1;
    BufferWrite(output,&co,&s);
    LineUnReadC(input,c);
  }
}

/********************************************************************/

static void handle_period(Buffer input,Buffer output)
{
  char   c;
  char   co = '.';
  size_t s;
  
  ddt(input  != NULL);
  ddt(output != NULL);
  
  if (LineEOF(input))
  {
    s = 1;
    BufferWrite(output,&co,&s);
    return;
  }

  LineReadC(input,&c);
  if (c == '.')
  {
    LineReadC(input,&c);
    if (c == '.')
      BufferFormatWrite(output,"","&#8230;");
    else
    {
      BufferFormatWrite(output,"","&#8229;");
      LineUnReadC(input,c);
    }
  }
  else
  {
    s = 1;
    BufferWrite(output,&co,&s);
    LineUnReadC(input,c);
  }
}

/*********************************************************************/

