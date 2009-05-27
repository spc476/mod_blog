
/*****************************************
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
*****************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

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

#include "wbtum.h"

/************************************************************************/

int main(int argc,char *argv[])
{
  Tumbler      tumbler;
  TumblerUnit  tu;
  char        *turl;
  int          rc;
  int          i;
  
  MemInit();
  BufferInit();
  DdtInit();
  CleanInit();
  
  for (i = 1 ; i < argc ; i++)
  {
    turl = argv[i];
    rc   = TumblerNew(&tumbler,&turl);
    if (rc != ERR_OKAY)
    {
      fprintf(stderr,"error on tumbler\n");
      continue;
    }

    printf("Flag: ");

    putchar( (tumbler->flags.file)     ? 'f' : ' ');
    putchar( (tumbler->flags.redirect) ? 'r' : ' ');
    putchar( (tumbler->flags.error)    ? 'e' : ' ');

    printf("\n");


    for (
          tu = (TumblerUnit)ListGetHead(&tumbler->units);
          NodeValid(&tu->node);
          tu = (TumblerUnit)NodeNext(&tu->node)
        )
    {
      printf("\tType: %2d Tumbler: ",tu->type);
      printf(" %d/%d/%d.%d",tu->entry[0],tu->entry[1],tu->entry[2],tu->entry[3]);
      if (tumbler->flags.file)
	printf(" %s",tu->file);
      printf("\n");
    }
    rc   = TumblerFree(&tumbler);
  }
  
  return(0);
}

