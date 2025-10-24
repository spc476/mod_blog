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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <syslog.h>
#include <cgilib8/util.h>

#include "frontend.h"

/*************************************************************************/

char *get_remote_user(void)
{
  /*-------------------------------------------------------
  ; A change in Apache 2.0.48:
  ;     Remember an authenticated user during internal redirects if the
  ;     redirection target is not access protected and pass it to scripts
  ;     using the REDIRECT_REMOTE_USER environment variable.  PR 10678,
  ;     11602
  ;
  ; Because of the way I'm doing this, this is affecting me, so I need to
  ; check both REMOTE_USER and REDIRECT_REMOTE_USER.
  ;------------------------------------------------------*/
  
  char const *name = getenv("REMOTE_USER");
  
  if (name == NULL)
  {
    name = getenv("REDIRECT_REMOTE_USER");
    if (name == NULL)
      name = "";
  }
  
  return strdup(name);
}

/************************************************************************/

static size_t breakline(char *dest[],size_t dsize,FILE *in)
{
  char    *line = NULL;
  char    *p;
  char    *colon;
  size_t   cnt;
  size_t   size = 0;
  ssize_t  rc;
  
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

/************************************************************************/

bool authenticate_author(Blog const *blog,Request *req)
{
  FILE   *in;
  char   *lines[10];
  size_t  cnt;
  
  assert(blog        != NULL);
  assert(req         != NULL);
  assert(req->author != NULL);
  
  if (blog->config.author.file == NULL)
    return strcmp(req->author,blog->config.author.name) == 0;
    
  in = fopen(blog->config.author.file,"r");
  if (in == NULL)
  {
    syslog(LOG_ERR,"%s: %s",blog->config.author.file,strerror(errno));
    return false;
  }
  
  while((cnt = breakline(lines,10,in)))
  {
    if ((blog->config.author.fields.uid < cnt) && (strcmp(req->author,lines[blog->config.author.fields.uid]) == 0))
    {
      if (blog->config.author.fields.name < cnt)
      {
        free(req->author);
        req->author = strdup(lines[blog->config.author.fields.name]);
        fclose(in);
        free(lines[0]); /* see comment below */
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

/**************************************************************************/
