/************************************************************************
*
* Copyright 2005 by Sean Conner.  All Rights Reserved.
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
*************************************************************************/

#define _GNU_SOURCE 1

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <syslog.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "frontend.h"
#include "globals.h"

#ifdef USE_GDBM
#  include <gdbm.h>
#endif

#ifdef USE_DB
#  include <limits.h>
#  include <sys/types.h>
#  include <db.h>
#endif

/*************************************************************************/

char *get_remote_user(void)
{
  char *name;
  
  /*-------------------------------------------------------
  ; A change in Apache 2.0.48:
  ;     Remember an authenticated user during internal redirects if the
  ;     redirection target is not access protected and pass it
  ;     to scripts using the REDIRECT_REMOTE_USER environment variable.
  ;     PR 10678, 11602
  ;
  ; Because of the way I'm doing this, this is affecting me, so I need
  ; to check both REMOTE_USER and REDIRECT_REMOTE_USER.
  ;------------------------------------------------------*/
  
  name = getenv("REMOTE_USER");
  if (name == NULL)
  {
    name = getenv("REDIRECT_REMOTE_USER");
    if (name == NULL)
      return strdup("");
  }
  
  return strdup(name);
}

/************************************************************************/

#if defined(USE_DB) || defined(USE_HTPASSWD)

  static size_t breakline(char **dest,size_t dsize,FILE *const in)
  {
    char   *line = NULL;
    char   *p;
    char   *colon;
    size_t  cnt;
    size_t  size = 0;
    ssize_t rc;
    
    assert(dest  != NULL);
    assert(dsize >  0);
    assert(in    != NULL);
    
    rc = getline(&line,&size,in);
    if ((rc == -1) || (emptynull_string(line)))
    {
      free(line);
      return 0;
    }
    
    if (line[rc - 1] == '\n')
      line[rc - 1] = '\0';
      
    p   = line;
    cnt = 0;
    
    do
    {
      dest[cnt] = p;
      colon = strchr(p,':');
      if (colon != NULL)
      {
        *colon = '\0';
        p      = colon + 1;
      }
      cnt++;
    } while ((colon != NULL) && (cnt < dsize));
    
    return cnt;
  }
  
#endif

/************************************************************************/

#if defined(USE_NONE)

  bool authenticate_author(Request req)
  {
    assert(req         != NULL);
    assert(req->author != NULL);
    
    return strcmp(req->author,c_author) == 0;
  }
  
  /************************************************************************/
  
#elif defined(USE_GDBM)

  bool authenticate_author(Request req)
  {
    GDBM_FILE list;
    datum     key;
    int       rc;
    
    assert(req         != NULL);
    assert(req->author != NULL);
    
    if (c_authorfile == NULL)
      return strcmp(req->author,c_author) == 0;
      
    list = gdbm_open(c_authorfile,DB_BLOCK,GDBM_READER,0,dbcritical);
    if (list == NULL)
      return false;
      
    key.dptr  = req->author;
    key.dsize = strlen(req->author) + 1;
    rc        = gdbm_exists(list,key);
    gdbm_close(list);
    return rc;
  }
  
  /***********************************************************************/
  
#elif defined (USE_DB)

  bool authenticate_author(Request req)
  {
    char   *lines[10];
    size_t  cnt;
    DB     *list;
    DBT     key;
    DBT     data;
    int     rc;
    
    assert(req         != NULL);
    assert(req->author != NULL);
    
    /*--------------------------------------------------------------
    ; this version will replace the login name with the full name,
    ; assuming it's defined in the database file as the (hardcoded)
    ; third field of the value portion returned.
    ;---------------------------------------------------------------*/
    
    if (c_authorfile == NULL)
      return  strcmp(req->author,c_author) == 0;
      
    list = dbopen(c_authorfile,O_RDONLY,0644,DB_HASH,NULL);
    if (list == NULL)
      return false;
      
    key.data = req->author;
    key.size = strlen(req->author);
    rc       = (list->get)(list,&key,&data,0);
    (list->close)(list);
    if (rc) return false;
    
    cnt        = breakline(lines,10,data.data);
    req->athor = strdup(lines[c_af_name]);
    free(lines[0]);
    
    return true;
  }
  
  /**********************************************************************/
  
#elif defined (USE_HTPASSWD)

  bool authenticate_author(Request req)
  {
    FILE   *in;
    char   *lines[10];
    size_t  cnt;
    
    assert(req         != NULL);
    assert(req->author != NULL);
    
    if (c_authorfile == NULL)
      return strcmp(req->author,c_author) == 0;
      
    in = fopen(c_authorfile,"r");
    if (in == NULL)
    {
      syslog(LOG_ERR,"%s: %s",c_authorfile,strerror(errno));
      return false;
    }
    
    while((cnt = breakline(lines,10,in)))
    {
      if ((c_af_uid < cnt) && (strcmp(req->author,lines[c_af_uid]) == 0))
      {
        /*----------------------------------------------
        ; A potential memory leak---see the comment above in
        ; the USE_DB version of this routine
        ;----------------------------------------------------*/
        
        if (c_af_name < cnt)
        {
          req->author = strdup(lines[c_af_name]);
          fclose(in);
          free(lines[0]); // see comment below
          return true;
        }
      }
      
      /*------------------------------------------------------------------
      ; Some tight coupling between this routine and breakline().  The first
      ; element of lines[] needs to be freed, but not the rest.
      ;--------------------------------------------------------------------*/
      
      free(lines[0]);
    }
    
    fclose(in);
    return false;
  }
  
#endif

/**************************************************************************/
