
/****************************************************************
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
*****************************************************************/

#include <stddef.h>
#include <string.h>

#include <cgi/ddt.h>
#include <cgi/memory.h>
#include <cgi/errors.h>

#include "array.h"

#define ARRAY_INIT_SIZE		32uL
#define ARRAY_INC_SIZE		32uL

/*******************************************************************/

Array (ArrayNew)(size_t size)
{
  Array a;
  
  ddt(size > 0);
  
  a.currsize = size * ARRAY_INIT_SIZE;
  a.size     = size;
  a.index    = 0;
  a.data     = MemAlloc(a.size);
  return(a);
}

/**********************************************************************/

Array (ArrayAdd)(Array a,void *data)
{
  size_t idx;
  size_t newsize;
  
  ddt(a.data     != NULL);
  ddt(a.size     >  0);
  ddt(a.currsize >= (a.index * a.size));
  
  idx = a.index * a.size;

  ddt(idx <= a.currsize);
    
  if (idx == a.currsize)
  {
    newsize    = a.currsize + (ARRAY_INC_SIZE * a.size);
    a.data     = MemGrow(a.data,a.currsize,newsize);
    a.currsize = newsize;
  }
  
  memcpy(((char *)a.data) + idx,data,a.size);
  a.index++;
  return(a);
}

/*************************************************************************/

size_t (ArraySize)(const Array a)
{
  ddt(a.data     != NULL);
  ddt(a.size     >  0);
  ddt(a.currsize >= (a.index * a.size));
  
  return(a.index);
}

/**************************************************************************/

void *(ArrayData)(const Array a)
{
  ddt(a.data     != NULL);
  ddt(a.size     >  0);
  ddt(a.currsize >= (a.index * a.size));
  
  return(a.data);
}

/**************************************************************************/

int (ArrayFree)(Array a)
{
  ddt(a.data     != NULL);
  ddt(a.size     >  0);
  ddt(a.currsize >= (a.index * a.size));
  
  MemFree(a.data,a.currsize);
  memset(&a,0,sizeof(Array));
  return(ERR_OKAY);
}

/**************************************************************************/

