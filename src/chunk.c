
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

#include <cgi/ddt.h>
#include <cgi/memory.h>
#include <cgi/util.h>

#include "conf.h"
#include "chunk.h"

/**********************************************************************/

static void chunk_readcallback(FILE *fpin,char *buff,size_t size)
{
  int c;
  
  ddt(fpin != NULL);
  ddt(buff != NULL);
  ddt(size >  0);
  
  *buff = '\0';
  
  while(size)
  {
    c = fgetc(fpin);
    if (c == EOF) return;
    
    if (c == '}')
    {
      c = fgetc(fpin);
      if (c == EOF) return;
      if (c == '%') return;
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
                              FILE                  *fpout,
                              char                  *cmd,
                              struct chunk_callback *pcc,
                              size_t                 scc,
                              void                  *data
                            )
{
  int i;

  ddt(fpout != NULL);
  ddt(cmd   != NULL);
  ddt(pcc   != NULL);
  ddt(scc   >  0);
  
  for (i = 0 ; i < scc ; i++)
  {
    if (strcmp(cmd,pcc[i].name) == 0)
    {
      (*pcc[i].callback)(fpout,data);
      return;
    }
  }
  fprintf(fpout,"%%{processing error - can't find [%s] }%%",cmd);
}

/************************************************************************/

static void chunk_handle(
                          FILE                  *fpin,
                          FILE                  *fpout,
                          struct chunk_callback *pcc,
                          size_t                 scc,
                          void                  *data
                        )
{
  char  cmdbuf[BUFSIZ];
  char *cmd;
  char *p;
  
  ddt(fpin  != NULL);
  ddt(fpout != NULL);
  ddt(pcc   != NULL);
  ddt(scc   >  0);
  
  chunk_readcallback(fpin,cmdbuf,BUFSIZ);
  
  for (p = cmdbuf ; (cmd = strtok(p," \t\v\r\n")) != NULL ; p = NULL )
  {
    chunk_docallback(fpout,cmd,pcc,scc,data);
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

int (ChunkProcess)(Chunk chunk,const char *name,FILE *fpout,void *data)
{
  char  fname[FILENAME_LEN];
  FILE *fpin;
  int   c;

  ddt(chunk != NULL);
  ddt(name  != NULL);
  ddt(fpout != NULL);
    
  sprintf(fname,"%s/%s",chunk->name,name);
  fpin = fopen(fname,"r");
  if (fpin == NULL)
    return(ErrorPush(AppErr,CHUNKPROCESS,CHUNKERR_OPEN,"$/$",chunk->name,name));

  while(1)
  {
    c = fgetc(fpin);
    if (c == '%')
    {
      c = fgetc(fpin);
      if (c == '{')
      {
        chunk_handle(fpin,fpout,chunk->cb,chunk->cbsize,data);
        continue;
      }

      fputc('%',fpout);
    }
    
    if (c == EOF) break;
    fputc(c,fpout);
  }
  
  fclose(fpin);
  return(ERR_OKAY);
}

/***********************************************************************/

int (ChunkFree)(Chunk chunk)
{
  ddt(chunk != NULL);
  
  MemFree(chunk->name,strlen(chunk->name) + 1);
  MemFree(chunk,sizeof(struct chunk));
  return(ERR_OKAY);
}

/**********************************************************************/

