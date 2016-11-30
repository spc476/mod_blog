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

#define _GNU_SOURCE	1

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <cgilib6/mail.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "blog.h"
#include "conversion.h"
#include "frontend.h"
#include "globals.h"
#include "fix.h"
#include "blogutil.h"

#ifdef EMAIL_NOTIFY
#  include <gdbm.h>
#endif

/*********************************************************************/

int entry_add(Request req)
{
  BlogEntry  entry;
  char      *p;
  
  assert(req != NULL);
  
  fix_entry(req);
  
  entry = BlogEntryNew(g_blog);
  
  if ((req->date == NULL) || (empty_string(req->date)))
  {
    /*----------------------------------------------------------------------
    ; if this is the case, then we need to ensure we update the main pages. 
    ; By calling BlogDatesInit() after we update we ensure we generate the
    ; content properly.
    ;---------------------------------------------------------------------*/
    
    entry->when = g_blog->now;
  }
  else
  {
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 0;
  entry->timestamp = g_blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->body      = req->body;
  
  if (c_authorfile) BlogLock(g_blog);
    
    BlogEntryWrite(entry);
    
  if (c_authorfile) BlogUnlock(g_blog);
  
  req->when = entry->when;
  free(entry);
  return 0;    
}

/************************************************************************/

void fix_entry(Request req)
{
  FILE   *out;
  FILE   *in;
  char   *tmp;
  size_t  size;
  
  assert(req != NULL);
  
  /*---------------------------------
  ; convert the title
  ;--------------------------------*/
  
  if (!empty_string(req->title))
  {
    tmp  = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->title,strlen(req->title),"r");
  
    buff_conversion(in,out);
    fclose(in);
    fclose(out);
    free(req->title);
    req->title = entity_conversion(tmp);
    free(tmp);
  }
  
  /*--------------------------------------
  ; convert the status
  ;-------------------------------------*/
  
  if (!empty_string(req->status))
  {
    tmp = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->status,strlen(req->status),"r");
    
    buff_conversion(in,out);
    fclose(in);
    fclose(out);
    free(req->status);
    req->status = entity_conversion(tmp);
    free(tmp);
  }  
  
  /*-------------------------------------
  ; convert body 
  ;--------------------------------------*/
  
  if (!empty_string(req->body))
  {
    tmp  = NULL;
    size = 0;
    out  = open_memstream(&tmp,&size);
    in   = fmemopen(req->body,strlen(req->body),"r");
  
    (*c_conversion)(in,out);
    fclose(in);
    fclose(out);
    free(req->body);
    req->body = tmp;
  }
}

/*************************************************************************/

void dbcritical(const char *msg)
{
  if (msg)
    fprintf(stderr,"critical error: %s\n",msg);
}

/************************************************************************/

#ifndef EMAIL_NOTIFY

void notify_emaillist(Request req __attribute__((unused)))
{
}

/************************************************************************/

#else

static void cb_email_title  (FILE *const,void *);
static void cb_email_url    (FILE *const,void *);
static void cb_email_author (FILE *const, void *);

static const struct chunk_callback m_emcallbacks[] =
{
  { "email.title"	, cb_email_title	} ,
  { "email.url" 	, cb_email_url		} ,
  { "email.author"	, cb_email_author	} ,
};

static const size_t m_emcbnum = sizeof(m_emcallbacks) / sizeof(struct chunk_callback);

/************************************************************************/

static ssize_t utf8_read(void *cookie,char *buffer,size_t bytes)
{
  FILE *realin = cookie;
  size_t s     = 0;
  
  assert(buffer != NULL);
  
  if (feof(realin))
    return 0;
  
  while(bytes)
  {
    int c = fgetc(realin);
    if (c == EOF) return s;
    
    if (c == '&')
    {
      char numbuf[10];
      
      if (bytes < 4)
      {
        ungetc(c,realin);
        return s;
      }
      
      c = fgetc(realin);
      assert(c == '#');
      
      for (size_t i = 0 ; i < sizeof(numbuf) ; i++)
      {
        c = fgetc(realin);
        if (!isdigit(c))
        {
          assert(c == ';');
          numbuf[i] = '\0';
          break;
        }
        numbuf[i] = c;
      }
      
      unsigned long val = strtoul(numbuf,NULL,10);
      assert(val <= 0x10FFFFuL);
      
      if (val < 0x80uL)
      {
        *buffer++ = (unsigned char)val;
        bytes--;
        s++;
      }
      else if (val < 0x800uL)
      {
        *(unsigned char *)buffer++ = (unsigned char)((val >> 6)   | 0xC0uL);
        *(unsigned char *)buffer++ = (unsigned char)((val & 0x3F) | 0x80uL);
        bytes -= 2;
        s     += 2;
      }
      else if (val < 0x10000uL)
      {
        *(unsigned char *)buffer++ = (unsigned char)(((val >> 12)         ) | 0xE0uL);
        *(unsigned char *)buffer++ = (unsigned char)(((val >>  6) & 0x3FuL) | 0x80uL);
        *(unsigned char *)buffer++ = (unsigned char)(((val      ) & 0x3FuL) | 0x80uL);
        bytes -= 3;
        s     += 3;
      }
      else if (val < 0x200000uL)
      {
        *(unsigned char *)buffer++ = (unsigned char)(((val >> 18)         ) | 0xF0uL);
        *(unsigned char *)buffer++ = (unsigned char)(((val >> 12) & 0x3FuL) | 0x80uL);
        *(unsigned char *)buffer++ = (unsigned char)(((val >>  6) & 0x3FuL) | 0x80uL);
        *(unsigned char *)buffer++ = (unsigned char)(((val      ) & 0x3Ful) | 0x80uL);
        bytes -= 4;
        s     += 4;
      }
      else
        assert(0);
    }
    else
    {
      *buffer++ = c;
      bytes--;
      s++;
    }
  }
  
  return s;
}

/************************************************************************/

static ssize_t qp_read(void *cookie,char *buffer,size_t bytes)
{
  FILE   *realin = cookie;
  size_t  s      = 0;
  
  assert(buffer != NULL);
  
  if (feof(realin))
    return 0;
  
  while(bytes)
  {
    int c = fgetc(realin);
    if (c == EOF) return s;
    
    if ((c == '=') || (c > '~'))
    {
      if (bytes < 3)
      {
        ungetc(c,realin);
        return s;
      }
      
      sprintf(buffer,"=%02X",c);
      buffer += 3;
      bytes  -= 3;
      s      += 3;
    }
    else
    {
      *buffer++ = c;
      bytes--;
      s++;
    }
  }
  
  return s;
}

/************************************************************************/

void notify_emaillist(Request req)
{
  GDBM_FILE  list;
  Chunk      templates;
  datum      nextkey;
  datum      key;
  datum      content;
  Email      email;
  FILE      *out;
  FILE      *in;
  char      *tmp  = NULL;
  size_t     size = 0;
  
  assert(req != NULL);
  
  list = gdbm_open((char *)c_emaildb,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
   
  email          = EmailNew();
  email->from    = strdup(c_email);
  email->subject = strdup(c_emailsubject);
  
  PairListCreate(&email->headers,"MIME-Version","1.0");
  PairListCreate(&email->headers,"Content-Type","text/plain; charset=UTF-8; format=flowed");
  PairListCreate(&email->headers,"Content-Transfer-Encoding","quoted-printable");
  
  templates = ChunkNew("",m_emcallbacks,m_emcbnum);
  out       = open_memstream(&tmp,&size);
  ChunkProcess(templates,c_emailmsg,out,req);
  ChunkFree(templates);
  fclose(out);
  
  in = fmemopen(tmp,size,"r");
  if (in != NULL)
  {
    FILE *inutf8 = fopencookie(in,"r",(cookie_io_functions_t)
                                      {
                                        utf8_read,
                                        NULL,
                                        NULL,
                                        NULL
                                      });
    if (inutf8 != NULL)
    {
      FILE *inqp = fopencookie(inutf8,"r",(cookie_io_functions_t)
                                          {
                                            qp_read,
                                            NULL,
                                            NULL,
                                            NULL
                                          });
      if (inqp != NULL)
      {      
        fcopy(email->body,inqp);
        key = gdbm_firstkey(list);
        
        while(key.dptr != NULL)
        {
          content = gdbm_fetch(list,key);
          if (content.dptr != NULL)
          {
            email->to = content.dptr;
            EmailSend(email);
            free(content.dptr);
          }
          
          nextkey = gdbm_nextkey(list,key);
          free(key.dptr);
          key = nextkey;
        }
        fclose(inqp);
      }
      fclose(inutf8);
    }
    fclose(in);
  }
  
  email->to = NULL;
  EmailFree(email);
  gdbm_close(list);
  free(tmp);
}

/*************************************************************************/

static void cb_email_title(FILE *const out,void *data)
{
  Request req = (Request)data;
  
  if (!empty_string(req->status))
    fprintf(out,"%s",req->status);
  else
    fprintf(out,"%s",req->title);
}

/*************************************************************************/

static void cb_email_url(FILE *const out,void *data)
{
  Request req = (Request)data;
  
  fprintf(
           out,
           "%s/%04d/%02d/%02d.%d",
           c_fullbaseurl,
           req->when.year,
           req->when.month,
           req->when.day,
           req->when.part
         );
}

/*************************************************************************/

static void cb_email_author(FILE *const out,void *data)
{
  Request req = (Request)data;
  
  fprintf(out,"%s",req->author);
}

/*************************************************************************/

#endif
