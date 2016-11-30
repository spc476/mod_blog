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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

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
  ;	Remember an authenticated user during internal redirects if the
  ;	redirection target is not access protected and pass it
  ;	to scripts using the REDIRECT_REMOTE_USER environment variable.
  ;	PR 10678, 11602
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
    DB  *list;
    DBT  key;
    DBT  data;
    int  rc;
    
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
    
    {
      char   *tmp;
      char   *p;
      char   *q;
      size_t  i;
      
      tmp = malloc(data.size + 1);
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
          free(tmp);
          return true;
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
      
      m_author = strdup(p);
      free(tmp);
    }
    
    return true;
  }

  /**********************************************************************/

#elif defined (USE_HTPASSWD)

  static size_t breakline(char **dest,size_t dsize,FILE *const in)
  {
    char  **tmp;
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
    
    if (line[size - 1] == '\n')
      line[size - 1] = '\0';
    
    tmp = malloc(sizeof(char *) * dsize);
    p   = line;
    cnt = 0;
    
    do
    {
      tmp[cnt] = p;
      colon    = strchr(p,':');
      if (colon != NULL)
      {
        *colon = '\0';
        p      = colon + 1;
      }
      cnt++;
    } while ((colon != NULL) && (cnt < dsize));
    
    dsize = cnt;
    
    for (cnt = 0 ; cnt < dsize ; cnt++)
      dest[cnt] = strdup(tmp[cnt]);
    
    free(tmp);
    free(line);
    return dsize;
  }
  
  /*------------------------------------------------------*/
  
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
      return false;
    
    while((cnt = breakline(lines,10,in)))
    {
      if (strcmp(req->author,lines[0]) == 0)
      {
        /*----------------------------------------------
        ; A potential memory leak---see the comment above in
        ; the USE_DB version of this routine
        ;----------------------------------------------------*/
        
        if (cnt >= 3)
        {
          req->name   = req->author;
          req->author = strdup(lines[2]);
          fclose(in);
          return true;
        }
      }
    }
    
    fclose(in);
    return false;
  }

#endif

/**************************************************************************/
