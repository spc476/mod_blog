/*********************************************
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
**********************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cgilib/ddt.h>
#include <cgilib/memory.h>
#include <cgilib/util.h>

#include "conf.h"
#include "chunk.h"

/**********************************************************************/

static void chunk_readcallback(Stream in,char *buff,size_t size)
{
  int c;
  
  ddt(in   != NULL);
  ddt(buff != NULL);
  ddt(size >  0);
  
  *buff = '\0';
  
  while(size)
  {
    c = StreamRead(in);
    if (c == IEOF) return;
    
    if (c == '}')
    {
      c = StreamRead(in);
      if (c == IEOF) return;
      if (c == '%')  return;
      *buff++ = '}';
    }
    
    *buff++ = tolower(c);
    *buff   = '\0';
    size--;
  }
  return;
}

/**************************************************************************/

static void chunk_docallback(
                              Stream                 out,
                              char                  *cmd,
                              struct chunk_callback *pcc,
                              size_t                 scc,
                              void                  *data
                            )
{
  int i;

  ddt(out   != NULL);
  ddt(cmd   != NULL);
  ddt(pcc   != NULL);
  ddt(scc   >  0);
  
  for (i = 0 ; i < scc ; i++)
  {
    if (strcmp(cmd,pcc[i].name) == 0)
    {
      (*pcc[i].callback)(out,data);
      return;
    }
  }
  LineSFormat(out,"$","%%{processing error - can't find [%a] }%%",cmd);
}

/************************************************************************/

static void chunk_handle(
			  Stream                 in,
			  Stream                 out,
                          struct chunk_callback *pcc,
                          size_t                 scc,
                          void                  *data
                        )
{
  char  cmdbuf[BUFSIZ];
  char *cmd;
  char *p;
  
  ddt(in    != NULL);
  ddt(out   != NULL);
  ddt(pcc   != NULL);
  ddt(scc   >  0);
  
  chunk_readcallback(in,cmdbuf,BUFSIZ);
  
  for (p = cmdbuf ; (cmd = strtok(p," \t\v\r\n")) != NULL ; p = NULL )
  {
    chunk_docallback(out,cmd,pcc,scc,data);
  }
}

/**************************************************************************/

int (ChunkNew)(Chunk *pch,const char *cname,struct chunk_callback *pcc,size_t scc)
{
  Chunk chunk;

  ddt(pch   != NULL);
  ddt(cname != NULL);
  ddt(pcc   != NULL);
  ddt(scc   >  0);
  
  chunk         = MemAlloc(sizeof(struct chunk));
  chunk->name   = dup_string(cname);
  chunk->cb     = pcc;
  chunk->cbsize = scc;  
  *pch          = chunk;
  
  return(ERR_OKAY);
}

/***********************************************************************/

int (ChunkProcess)(Chunk chunk,const char *name,Stream out,void *data)
{
  char   fname[FILENAME_LEN];
  Stream in;
  int    c;

  ddt(chunk != NULL);
  ddt(name  != NULL);
  ddt(out   != NULL);
    
  sprintf(fname,"%s/%s",chunk->name,name);
  
  in = FileStreamRead(fname);
  if (in == NULL)
    return(ERR_ERR);

  while(!StreamEOF(in))
  {
    c = StreamRead(in);
    if (c == IEOF) break;
    if (c == '%')
    {
      c = StreamRead(in);
      if (c == '{')
      {
        chunk_handle(in,out,chunk->cb,chunk->cbsize,data);
        continue;
      }

      StreamWrite(out,'%');
    }
    StreamWrite(out,c);    
  }

  StreamFree(in);  
  return(ERR_OKAY);
}

/***********************************************************************/

int (ChunkFree)(Chunk chunk)
{
  ddt(chunk != NULL);
  
  MemFree(chunk->name);
  MemFree(chunk);
  return(ERR_OKAY);
}

/**********************************************************************/

