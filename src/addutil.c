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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <syslog.h>
#include <gdbm.h>

#include <cgilib8/mail.h>
#include <cgilib8/util.h>
#include <cgilib8/chunk.h>

#include "conversion.h"
#include "backend.h"
#include "blogutil.h"

#define DB_BLOCK 1024

/*********************************************************************/

bool entry_add(Blog *blog,Request *req)
{
  BlogEntry *entry;
  bool       rc = true;
  
  assert(blog != NULL);
  assert(req  != NULL);
  
  fix_entry(req);
  
  if (blog->config.prehook != NULL)
  {
    FILE *fp;
    char  fnbody[L_tmpnam];
    char  fnmeta[L_tmpnam];
    
    tmpnam(fnbody);
    tmpnam(fnmeta);
    
    fp = fopen(fnbody,"w");
    if (fp != NULL)
    {
      fputs(req->body,fp);
      fclose(fp);
    }
    else
    {
      syslog(LOG_ERR,"entry_add: tmp-body: %s",strerror(errno));
      return false;
    }
    
    fp = fopen(fnmeta,"w");
    if (fp != NULL)
    {
      char ts[12];
      
      if (req->date == NULL)
        snprintf(ts,sizeof(ts),"%04d/%02d/%02d",blog->now.year,blog->now.month,blog->now.day);
      else
        snprintf(ts,sizeof(ts),"%s",req->date);
        
      fprintf(
        fp,
        "Author: %s\n"
        "Title: %s\n"
        "Class: %s\n"
        "Status: %s\n"
        "Date: %s\n"
        "Adtag: %s\n"
        "\n",
        req->author,
        req->title,
        req->class,
        req->status,
        ts,
        req->adtag
      );
      fclose(fp);
    }
    else
    {
      remove(fnbody);
      syslog(LOG_ERR,"entry_add: tmp-meta: %s",strerror(errno));
      return false;
    }
    
    rc = run_hook("entry-pre-hook",(char const *[]){ blog->config.prehook , fnbody , fnmeta , NULL });
    
    remove(fnmeta);
    remove(fnbody);
    
    if (!rc)
      return rc;
  }
  
  entry = BlogEntryNew(blog);
  
  if (emptynull_string(req->date))
    entry->when = blog->now;
  else
  {
    char *p;
    
    entry->when.year  = strtoul(req->date,&p,10); p++;
    entry->when.month = strtoul(p,        &p,10); p++;
    entry->when.day   = strtoul(p,        &p,10);
  }
  
  entry->when.part = 0;
  entry->timestamp = blog->tnow;
  entry->title     = req->title;
  entry->class     = req->class;
  entry->status    = req->status;
  entry->author    = req->author;
  entry->adtag     = req->adtag;
  entry->body      = req->body;
  
  BlogEntryWrite(entry);
  
  req->when = entry->when;
  
  if (blog->config.posthook != NULL)
  {
    /*---------------------------------------------------------------
    ; XXX maybe in the future move email notifications to this hook?
    ;----------------------------------------------------------------*/
    
    char url[1024];
    
    snprintf(
      url,
      sizeof(url),
      "%s/%04d/%02d/%02d.%d",
      blog->config.url,
      entry->when.year,
      entry->when.month,
      entry->when.day,
      entry->when.part
    );
    
    rc = run_hook("entry-post-hook",(char const *[]){ blog->config.posthook , url , NULL });
  }
  
  if (req->f.email)
    notify_emaillist(entry);
    
  free(entry);
  return rc;
}

/************************************************************************/

void fix_entry(Request *req)
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
    
    (*req->conversion)(in,out);
    fclose(in);
    fclose(out);
    free(req->body);
    req->body = tmp;
  }
}

/*************************************************************************/

static void dbcritical(char const *msg)
{
  assert(msg != NULL);
  syslog(LOG_ERR,"gdbm_open() = %s",msg);
}

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

struct qpcookie
{
  FILE *fpin;
  size_t l;
};

static ssize_t qp_read(void *cookie,char *buffer,size_t bytes)
{
  struct qpcookie *qpc = cookie;
  FILE   *realin       = qpc->fpin;
  size_t  s            = 0;
  
  assert(buffer != NULL);
  
  if (feof(realin))
    return 0;
    
  while(bytes)
  {
    if (qpc->l >= 68)
    {
      if (bytes < 2) return s;
      *buffer++ = '=';
      *buffer++ = '\n';
      s        += 2;
      bytes    -= 2;
      qpc->l    = 0;
      continue;
    }
    
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
      qpc->l += 3;
    }
    else
    {
      *buffer++ = c;
      bytes--;
      s++;
      qpc->l++;
    }
  }
  
  return s;
}

/************************************************************************/

static void cb_email_title(FILE *out,void *data)
{
  BlogEntry *entry = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  if (!empty_string(entry->status))
    fprintf(out,"%s",entry->status);
  else
    fprintf(out,"%s",entry->title);
}

/*************************************************************************/

static void cb_email_url(FILE *out,void *data)
{
  BlogEntry *entry = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  
  fprintf(
           out,
           "%s%04d/%02d/%02d.%d",
           entry->blog->config.url,
           entry->when.year,
           entry->when.month,
           entry->when.day,
           entry->when.part
         );
}

/*************************************************************************/

static void cb_email_author(FILE *out,void *data)
{
  BlogEntry *entry = data;
  
  assert(out  != NULL);
  assert(data != NULL);
  fprintf(out,"%s",entry->author);
}

/*************************************************************************/

void notify_emaillist(BlogEntry *entry)
{
  static struct chunk_callback const emcallbacks[] =
  {
    { "email.title"  , cb_email_title  } ,
    { "email.url"    , cb_email_url    } ,
    { "email.author" , cb_email_author } ,
  };
  
  GDBM_FILE  list;
  Chunk      templates;
  Email      email;
  FILE      *out;
  FILE      *in;
  char      *tmp  = NULL;
  size_t     size = 0;
  
  assert(entry != NULL);
  assert(entry->valid);
  
  list = gdbm_open((char *)entry->blog->config.email.list,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
    
  email          = EmailNew();
  email->from    = strdup(entry->blog->config.author.email);
  email->subject = strdup(entry->blog->config.email.subject);
  
  PairListCreate(&email->headers,"MIME-Version","1.0");
  PairListCreate(&email->headers,"Content-Type","text/plain; charset=UTF-8; format=flowed");
  PairListCreate(&email->headers,"Content-Transfer-Encoding","quoted-printable");
  
  templates = ChunkNew("",emcallbacks,sizeof(emcallbacks) / sizeof(emcallbacks[0]));
  out       = open_memstream(&tmp,&size);
  ChunkProcess(templates,entry->blog->config.email.message,out,entry);
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
      struct qpcookie qpc;
      
      qpc.l      = 0;
      qpc.fpin   = inutf8;
      FILE *inqp = fopencookie(&qpc,"r",(cookie_io_functions_t)
                                          {
                                            qp_read,
                                            NULL,
                                            NULL,
                                            NULL
                                          });
      if (inqp != NULL)
      {
        datum key;
        datum nextkey;
        
        fcopy(email->body,inqp);
        key = gdbm_firstkey(list);
        
        while(key.dptr != NULL)
        {
          datum content = gdbm_fetch(list,key);
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
