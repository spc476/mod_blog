
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
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <gdbm.h>

#include <cgi/memory.h>
#include <cgi/buffer.h>
#include <cgi/ddt.h>
#include <cgi/clean.h>
#include <cgi/errors.h>
#include <cgi/nodelist.h>
#include <cgi/htmltok.h>
#include <cgi/util.h>
#include <cgi/pair.h>
#include <cgi/cgi.h>

#include "conf.h"
#include "blog.h"
#include "conversion.h"
#include "timeutil.h"
#include "doi_util.h"
#include "globals.h"

#ifdef USE_DB
#  include <sys/types.h>
#  include <limits.h>
#  include <db.h>
#endif

#define DB_BLOCK		1024

#define MAILSETUPDATA		(ERR_APP + 1)
#define CGISETUPDATA		(ERR_APP + 2)
#define FILESETUPDATA		(ERR_APP + 3)
#define CBBODY			(ERR_APP + 4)

#define SETUPERR_INPUT		(ERR_APP + 1)
#define SETUPERR_AUTH		(ERR_APP + 2)
#define SETUPERR_MEMORY		(ERR_APP + 3)

/***********************************************************************/

static int		 mail_setup_data	(void);
static int		 cgi_setup_data		(void);
static int		 file_setup_data	(char *);
static int		 mailfile_readdata	(void);
static int		 authenticate_author	(void);
static void		 fix_entry		(void);
static void		 collect_body		(Buffer,Buffer);
static void		 do_error		(Cgi,char *);
static void		 notify_weblogcom	(void);
static void		 notify_emaillist	(void);
static void		 dbcritical		(char *);

/**********************************************************************/

static Cgi         m_cgi;
static char       *m_cgibody;
static Buffer      m_rinput;	/* cleanup - remove in future version */
static Buffer      m_linput;	/* cleanup - remove in future version */
static const char *m_author = "Sean Conner";
static       char *m_title  = "[Generic and Pithy Title goes here]";
static const char *m_class  = "online journal, blog, weblog, generic stuff";
static const char *m_date;

/***********************************************************************/

int main(int argc,char *argv[])
{
  int       rc;
  struct tm date;
  BlogDay   day;
  BlogEntry entry;
  int       lock;

  MemInit();
  BufferInit();
  DdtInit();
  CleanInit();

  while(g_debug)	/* possibly pause so we can attach a debugger 	*/
    ;			/* to the running process			*/

  if (argc == 1) 
  {
    fprintf(stderr,"usage: %s config [entry]\n",argv[0]);
    return(EXIT_FAILURE);
  }

  GlobalsInit(argv[1]);
  BlogInit();
  if (g_authorfile) lock = BlogLock(g_lockfile);
 
  if (strstr(argv[0],".cgi") != NULL)
    rc = cgi_setup_data();
  else if (argc == 2)
    rc = mail_setup_data();
  else 
    rc = file_setup_data(argv[2]);
 
  if (rc != ERR_OKAY)
  {
    ErrorLog();
    ErrorClear();
    return(1);
  }

  if ((m_date == NULL) || (empty_string(m_date)))
  {
    time_t     t;
    struct tm *now;
    
    t    = time(NULL);
    now  = localtime(&t);
    date = *now;
  }
  else
  {
    char *p;
 
    date.tm_sec   = 0;
    date.tm_min   = 0;
    date.tm_hour  = 1;
    date.tm_wday  = 0;
    date.tm_yday  = 0;
    date.tm_isdst = -1;
    date.tm_year  = strtoul(m_date,&p,10); p++;
    date.tm_mon   = strtoul(p,&p,10); p++;
    date.tm_mday  = strtoul(p,NULL,10); 
    tm_to_tm(&date);
  }

  fix_entry();
 
  BlogDayRead(&day,&date);
  BlogEntryNew(&entry,m_title,m_class,m_author,m_cgibody,strlen(m_cgibody));
  BlogDayEntryAdd(day,entry);
  BlogDayWrite(day);
  BlogDayFree(&day);
 
  if (g_backend)
  {
    char bufcmd[BUFSIZ];
    
    sprintf(bufcmd,"%s %s",g_backend,argv[1]);
    system(bufcmd);
  }
  else
    return(ErrorPush(AppErr,ADDENTRY_MAIN,APPERR_NOBACKEND,""));

  if (g_authorfile)  BlogUnlock(lock);
  if (g_weblogcom)   notify_weblogcom();
  if (g_emailupdate) notify_emaillist();
 
  return(0);
}
 
/********************************************************************/

static void fix_entry(void)
{
  Buffer in;
  Buffer lin;
  Buffer out;
  char   tmpbuf[65536UL];
  int    rc;
 
  memset(tmpbuf,0,sizeof(tmpbuf));
  rc = MemoryBuffer(&out,tmpbuf,sizeof(tmpbuf));
  rc = MemoryBuffer(&in,m_title,strlen(m_title));
  BufferIOCtl(in,CM_SETSIZE,strlen(m_title));
  LineBuffer(&lin,in);
  buff_conversion(lin,out,QUOTE_SMART);

  BufferFree(&lin);
  BufferFree(&in); 
  BufferFree(&out);
 
  MemFree(m_title,strlen(m_title) + 1);
  m_title = dup_string(tmpbuf);

  memset(tmpbuf,0,sizeof(tmpbuf));
  rc = MemoryBuffer(&out,tmpbuf,sizeof(tmpbuf));
  rc = MemoryBuffer(&in,m_cgibody,strlen(m_cgibody));
  BufferIOCtl(in,CM_SETSIZE,strlen(m_cgibody));
 
  (*g_conversion)("body",in,out);

  BufferFree(&in); 
  BufferFree(&out);
 
  MemFree(m_cgibody,strlen(m_cgibody) + 1);
  m_cgibody = dup_string(tmpbuf);
  
}

/************************************************************************/

static int mail_setup_data(void)
{
  char   data[BUFSIZ];
  size_t size;
  int    rc;

  rc = FHBuffer(&m_rinput,0);
  if (rc != ERR_OKAY)
  {
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_INPUT,""));
  }

  rc = LineBuffer(&m_linput,m_rinput);
  if (rc != ERR_OKAY)
  {
    BufferFree(&m_rinput);
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_INPUT,""));
  }

  size = BUFSIZ;
  LineRead(m_linput,data,&size);	/* skip Unix 'From ' line */

  /*---------------------------------------------
  ; skip the header section---we just ignore it
  ;---------------------------------------------*/

  do
  {
    size = BUFSIZ;
    LineRead822(m_linput,data,&size);
  } while((size != 0) && (!empty_string(data)));

  return(mailfile_readdata());
}

/********************************************************************/

static int file_setup_data(char *filename)
{
  int rc;
  
  ddt(filename != NULL);
 
  rc = FileBuffer(&m_rinput,filename,MODE_READ);
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,FILESETUPDATA,SETUPERR_INPUT,"$",filename));

  rc = LineBuffer(&m_linput,m_rinput);
  if (rc != ERR_OKAY)
  {
    BufferFree(&m_rinput);
    return(ErrorPush(AppErr,FILESETUPDATA,SETUPERR_INPUT,""));
  }

  return(mailfile_readdata());
}

/********************************************************************/

static int mailfile_readdata(void)
{
  char         data       [BUFSIZ];
  char         megabuffer [65536UL];
  char        *pt;
  struct pair *ppair;
  size_t       size;
  Buffer       output;
  int          rc;

  memset(megabuffer,0,sizeof(megabuffer));
 
  /*--------------------------------------------
  ; now read the user supplied header
  ;--------------------------------------------*/

  while(TRUE)
  {
    size = BUFSIZ;
    LineRead822(m_linput,data,&size);

    /*------------------------------------
    ; break this up, it might not be working
    ; correctly ... 
    ;------------------------------------*/

    if (size == 0) break;
    if (empty_string(data)) break;

    pt           = data;
    ppair        = PairNew(&pt,':','\0');
    ppair->name  = up_string(trim_space(ppair->name));
    ppair->value = trim_space(ppair->value);

    if (strcmp(ppair->name,"AUTHOR") == 0)
      m_author = dup_string(ppair->value);
    else if (strcmp(ppair->name,"TITLE") == 0)
      m_title = dup_string(ppair->value);
    else if (strcmp(ppair->name,"SUBJECT") == 0)
      m_title = dup_string(ppair->value);
    else if (strcmp(ppair->name,"CLASS") == 0)
      m_class = dup_string(ppair->value);
    else if (strcmp(ppair->name,"DATE") == 0)
      m_date = dup_string(ppair->value);
    else if (strcmp(ppair->name,"EMAIL") == 0)
    {
      if (strcmp(up_string(ppair->value),"NO") == 0)
        g_emailupdate = FALSE;
    }
    else if (strcmp(ppair->name,"FILTER") == 0)
    {
      up_string(ppair->value);
      if (strcmp(ppair->value,"TEXT") == 0)
        g_conversion = text_conversion;
      else if (strcmp(ppair->value,"MIXED") == 0)
        g_conversion = mixed_conversion;
      else if (strcmp(ppair->value,"HTML") == 0)
        g_conversion = html_conversion;
    }
    PairFree(ppair);
  }

  /*if (strcmp(m_author,g_author) != 0)*/
  if (authenticate_author() == FALSE)
  {
    BufferFree(&m_linput);
    BufferFree(&m_rinput);
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_AUTH,""));
  }

  rc = MemoryBuffer(&output,megabuffer,sizeof(megabuffer));
  if (rc != ERR_OKAY)
    return(ErrorPush(AppErr,MAILSETUPDATA,SETUPERR_MEMORY,""));

  collect_body(output,m_linput);
  BufferFree(&output);
  m_cgibody = dup_string(megabuffer);

  return(ERR_OKAY);
}

/************************************************************************/

static int cgi_setup_data(void)
{
  char *t;
 
  if (CgiNew(&m_cgi,NULL) != ERR_OKAY)
  {
    do_error(m_cgi,"errors new");
    return(ErrorPush(AppErr,CGISETUPDATA,SETUPERR_INPUT,""));

  }

  CgiListMake(m_cgi);

  t = CgiListGetValue(m_cgi,"email");
  if (t && !empty_string(t))
  {
    if (strcmp(up_string(t),"NO") == 0)
      g_emailupdate = FALSE;
  }

  t = CgiListGetValue(m_cgi,"filter");
  if (t && !empty_string(t))
  {
    up_string(t);
    if (strcmp(t,"TEXT") == 0)
      g_conversion = text_conversion;
    else if (strcmp(t,"MIXED") == 0)
      g_conversion = mixed_conversion;
    else if (strcmp(t,"HTML") == 0)
      g_conversion = html_conversion ;
  }

  m_author  = CgiListGetValue(m_cgi,"author");
  m_title   = CgiListGetValue(m_cgi,"title");
  m_class   = CgiListGetValue(m_cgi,"class");
  m_date    = CgiListGetValue(m_cgi,"date");
  m_cgibody = CgiListGetValue(m_cgi,"body");

  if ((m_author == NULL) || (empty_string(m_author)))
  {
    m_author = spc_getenv("REMOTE_USER");
  }
 
  if (
       (m_author == NULL)
       || (empty_string(m_author))
       || (m_title == NULL) 
       || (empty_string(m_title))
       || (m_class == NULL)
       || (empty_string(m_class))
       || (m_cgibody == NULL)
       || (empty_string(m_cgibody))
     )
  {
    do_error(m_cgi,"errors-missing");
    return(ErrorPush(AppErr,CGISETUPDATA,SETUPERR_INPUT,""));
  }
 
  if (authenticate_author() == FALSE)
  {
    do_error(m_cgi,"errors-author not authenticated");
    return(ErrorPush(AppErr,CGISETUPDATA,SETUPERR_INPUT,""));
  }

#if 0
  rc = MemoryBuffer(&m_rinput,m_cgibody,strlen(m_cgibody));
  if (rc != ERR_OKAY)
  {
    do_error(m_cgi,"errors memory");
    return(ErrorPush(AppErr,CGISETUPDATA,SETUPERR_INPUT,""));
  }
  BufferIOCtl(m_rinput,CM_SETSIZE,strlen(m_cgibody));
  rc = LineBuffer(&m_linput,m_rinput);
  if (rc != ERR_OKAY)
  {
    BufferFree(&m_rinput);
    do_error(m_cgi,"errors input");
    return(ErrorPush(AppErr,CGISETUPDATA,SETUPERR_INPUT,""));
  }
#endif

  {
    char output[BUFSIZ];
    
    sprintf(
    	output,
    	"<p>The entry was created and saved okay.</p>"
    	"<ul>\n"
    	"  <li><a href='%s'>Back to entry form</a></li>"
    	"  <li><a href='%s'>View new entry</a></li>"
    	"</ul>\n",
    	spc_getenv("HTTP_REFERER"),
    	g_fullbaseurl
    );

    do_error(m_cgi,output);
  }
  return(ERR_OKAY);
}

/***********************************************************************/

static void collect_body(Buffer output,Buffer input)
{
  HtmlToken token;
  int       rc;
  int       t;

  ddt(output != NULL);
  ddt(input  != NULL);
 
  rc = HtmlParseNew(&token,input);
  if (rc != ERR_OKAY)
  {
    ErrorPush(AppErr,CBBODY,rc,"");
    ErrorLog();
    ErrorClear();
    return;
  }
 
  while((t = HtmlParseNext(token)) != T_EOF)
  {
    if (t == T_TAG)
    {
      struct pair *pp;
 
      if (strcmp(HtmlParseValue(token),"/HTML") == 0) break;
      if (strcmp(HtmlParseValue(token),"/BODY") == 0) break;
      BufferFormatWrite(output,"$","<%a",HtmlParseValue(token));
      for (
            pp = HtmlParseFirstOption(token);
            NodeValid(&pp->node);
            pp = (struct pair *)NodeNext(&pp->node)
          )
      {
        if (pp->name && !empty_string(pp->name))
          BufferFormatWrite(output,"$"," %a",pp->name);
        if (pp->value && !empty_string(pp->value))
          BufferFormatWrite(output,"$","=\"%a\"",pp->value);
      }
      BufferFormatWrite(output,"",">");
    }
    else if (t == T_STRING)
    {
      size_t s = strlen(HtmlParseValue(token));
      BufferWrite(output,HtmlParseValue(token),&s);
    }
    else if (t == T_COMMENT)
    {
      BufferFormatWrite(output,"$","<!%a>",HtmlParseValue(token));
    }
  }

  HtmlParseFree(&token);    
}

/***********************************************************************/

static void do_error(Cgi cgi,char *msg)
{
  CgiOutHtml(cgi);
  BufferFormatWrite(
  	CgiBufferOut(cgi),
  	"$",
        "<html>\n"
        "<head>\n"
        "  <title>Output</title>\n"
        "</head>\n"
        "<body>\n"
        "%a\n"
        "</body>\n"
        "</html>\n"
        "\n",
        msg
        );
}

/************************************************************************/

static char *headers[] = 
{
  "",
  "User-agent: mod_blog/R13 (addentry-1.22 http://boston.conman.org/about)\r\n",
  "Accept: text/html\r\n",
  "\r\n",
  NULL
};

static void notify_weblogcom(void)
{
  URLHTTP url;
  HTTP    conn;
  char    *query;
  char    *name;
  char    *blogurl;
  char    *nfile;
  int      rc;
  char     buffer[BUFSIZ];
  size_t   size;
  size_t   fsize;
  int      fh;

  fsize      = 6 + strlen(g_email) + 2 + 1;
  headers[0] = MemAlloc(fsize);
  sprintf(headers[0],"From: %s\r\n",g_email);

  name    = UrlEncodeString(g_name);
  blogurl = UrlEncodeString(g_fullbaseurl);

  size = 5	/* name = */
         + strlen(name)
         + 1	/* & */
         + 4	/* url= */
         + strlen(blogurl)
         + 1;	/* EOS */

  query = MemAlloc(size);
  sprintf(query,"name=%s&url=%s",name,blogurl);

  UrlNew((URL *)&url,g_weblogcomurl);
 
  nfile = MemAlloc(strlen(url->file) + 1 + strlen(query) + 1);
  sprintf(nfile,"%s?%s",url->file,query);
  MemFree(url->file,strlen(url->file) + 1);
  MemFree(query,size);
  url->file = nfile;
 
  rc = HttpOpen(&conn,GET,url,(const char **)headers);
  if (rc != ERR_OKAY)
  {
    ErrorClear();
    return;
  }
 
  BufferIOCtl(HttpBuffer(conn),CF_HANDLE,&fh);
 
  while(read(fh,buffer,sizeof(buffer)) > 0)
    ;
  HttpClose(&conn);
  UrlFree((URL *)&url);
  MemFree(headers[0],fsize);
}

/*************************************************************************/

static void notify_emaillist(void)
{
  GDBM_FILE list;
  datum     key;
  datum     content;
 
  list = gdbm_open((char *)g_emaildb,DB_BLOCK,GDBM_READER,0,dbcritical);
  if (list == NULL)
    return;
 
  key = gdbm_firstkey(list);
 
  while(key.dptr != NULL)
  {
    content = gdbm_fetch(list,key);
    if (content.dptr != NULL)
    {
      send_message(g_email,NULL,content.dptr,g_emailsubject,g_emailmsg);
    }
    key = gdbm_nextkey(list,key);
  }
  gdbm_close(list);
}

/*************************************************************************/

static void dbcritical(char *msg)
{
  if (msg)
    ddtlog(ddtstream,"$","critical error [%a]",msg);
}

/*************************************************************************/

#ifdef USE_GDBM

  static int authenticate_author(void)
  {
    GDBM_FILE list;
    datum     key;
    int       rc;

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);

    list = gdbm_open(g_authorfile,DB_BLOCK,GDBM_READER,0,dbcritical);
    if (list == NULL)
      return(FALSE);

    key.dptr  = m_author;
    key.dsize = strlen(m_author) + 1;
    rc        = gdbm_exists(list,key);
    gdbm_close(list);
    return(rc);
  }

#elif defined (USE_DB)

  static int authenticate_author(void)
  {
    DB  *list;
    DBT  key;
    DBT  data;
    int  rc;

    /*--------------------------------------------------------------
    ; this version will replace the login name with the full name,
    ; assuming it's defined in the database file as the (hardcoded)
    ; third field of the value portion returned.
    ;---------------------------------------------------------------*/

    ddt(author != NULL);

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);

    list = dbopen(g_authorfile,O_RDONLY,0644,DB_HASH,NULL);
    if (list == NULL)
      return(FALSE);

    key.data = m_author;
    key.size = strlen(m_author);
    rc       = (list->get)(list,&key,&data,0);
    (list->close)(list);
    if (rc) return(FALSE);
    
    {
      char   *tmp;
      char   *p;
      char   *q;
      size_t  i;

      tmp = MemAlloc(data.size + 1);
      memset(tmp,0,data.size + 1);
      memcpy(tmp,data.data,data.size);
 
      /*----------------------------------------------------------
      ; For now, we assume that if using Berkeley DB, we are doing
      ; so because we want to use the DB file used by Apache, and
      ; that's set up as key => passwd ':' group ':' other
      ; so we look for two colons, and take what is past there as
      ; the real name.  
      ;-----------------------------------------------------------*/
 
      for (i = 0 , p = tmp ; i < 2 ; i++)
      {
        p = strchr(p,':');
        if (p == NULL)
        {
          MemFree(tmp,key.size + 1);
          return(TRUE);
        }
        p++;
      }
 
      /*----------------------------------------------------------
      ; there may be more fields, so check for the end of this field,
      ; and if so marked, replace it with an end of string marker.
      ;-----------------------------------------------------------*/
 
      q = strchr(p,':');
      if (q) *q = '\0';
 
      /*------------------------------------------------------
      ; There is a potential memory leak here, as m_author may
      ; have been allocated earlier.  But since it's constantly
      ; defined to begin with, and there is no real portable way
      ; to determine if the pointer has been allocated at run time
      ; we can't really test it, so we loose some memory here.
      ;
      ; This is expected, but since this program doesn't run
      ; infinitely, this is somewhat okay.
      ;------------------------------------------------------*/
 
      m_author = dup_string(p);
      MemFree(tmp,key.size + 1);
    }

    return(TRUE);
  }

#elif defined (USE_HTPASSWD)

  static size_t breakline(char **dest,size_t dsize,FILE *fpin)
  {
    static char buffer[BUFSIZ];	/* non-thread-safe! */
    char        *p;
    char        *nl;
    size_t       cnt;
    
    p = fgets(buffer,sizeof(buffer),fpin);
    if (p == NULL) return(0);

    nl = strchr(buffer,'\n');	/* remove trailing newline */
    if (nl) *nl = '\0';

    for (cnt = 0 ; cnt < dsize ; cnt++)
    {
      dest[cnt] = p;
      p = strchr(p,':');
      if (p == NULL) return(cnt + 1);
      *p++ = '\0';
    }
    return(cnt);
  }

  /*------------------------------------------------------*/

  static int authenticate_author(void)
  {
    FILE   *fpin;
    char   *lines[10];	/* 10 fields max */
    size_t  cnt;

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);
 
    fpin = fopen(g_authorfile,"r");
    if (fpin == NULL)
      return(FALSE);
 
    while((cnt = breakline(lines,10,fpin)))
    {
      if (strcmp(m_author,lines[0]) == 0)
      {
        /*--------------------------------------------------
        ; A potential memory leak---see the comment above in 
        ; the USE_DB version of this routine
        ;---------------------------------------------------*/
        
        if (cnt >= 3)
          m_author = dup_string(lines[2]);

        fclose(fpin);
        return(TRUE);
      }
    }
    fclose(fpin);
    return(FALSE);
  }

#endif

/**************************************************************************/

