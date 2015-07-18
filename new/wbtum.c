/*********************************************************************
*
* Copyright 2015 by Sean Conner.  All Rights Reserved.
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
**********************************************************************/

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "wbtum.h"

/************************************************************************/

struct value
{
  const char *txt;
  ptrdiff_t   len;
  int         val;
};

/************************************************************************/

static bool parse_num(
        struct value  *pv,
        const char   **p,
        int            low,
        int            high
)
{
  long val;
  
  assert(pv != NULL);
  assert(p  != NULL);
  assert(*p != NULL);
  
  if (!isdigit(**p))
    return false;
  
  pv->txt = *p;
  errno   = 0;
  val     = strtol(*p,(char **)p,10);
  
  if (errno != 0)
    return false;
  
  if (val < (long)low)
    return false;
  if (val > (long)high)
    return false;
  
  assert(val >= (long)INT_MIN);
  assert(val <= (long)INT_MAX);
  
  pv->val = (int)val;
  pv->len = *p - pv->txt;
  
  return true;
}

/************************************************************************/

bool tumbler_new(tumbler__s *tum,const char *text)
{
  struct value u1;
  //struct value u2;
  //struct value u3;
  //struct value u4;
  
  assert(tum  != NULL);
  assert(text != NULL);
  
  tum->start.year  = tum->stop.year  = 0;
  tum->start.month = tum->stop.month = 1;
  tum->start.day   = tum->stop.day   = 1;
  tum->start.part  = tum->stop.part  = 1;
  tum->ustart      = tum->ustop      = UNIT_YEAR;
  tum->file        = false;
  tum->redirect    = false;
  tum->range       = false;
  tum->filename[0] = '\0';
  
  /*-------------------------------------------------
  ; parse year
  ;-------------------------------------------------*/
  
  if (!parse_num(&u1,&text,1999,2200))
    return false;
  
  tum->start.year = tum->stop.year = u1.val;
  tum->ustart     = tum->ustop     = UNIT_YEAR;
  
  if (*text == '\0')
    return true;
  
  if (*text == '-')
    goto tumbler_new_range;
  
  if (*text != '/')
    return false;
  
  text++;
  if (*text == '\0')
    return tum->redirect |= true;
  
  /*-------------------------------------------------
  ; parse month
  ;-------------------------------------------------*/
  
  if (!parse_num(&u1,&text,1,12))
    return false;
  
  tum->start.month = tum->stop.month = u1.val;
  tum->ustart      = tum->ustop      = UNIT_MONTH;
  
  if (u1.len == 1)
    tum->redirect |= true;
  
  if (*text == '\0')
    return true;
  
  if (*text == '-')
    goto tumbler_new_range;
  
  if (*text != '/')
    return false;
  
  text++;
  
  if (*text == '\0')
    return tum->redirect |= true;
  
  /*--------------------
  ; parse day
  ;---------------------*/
  
  if (!parse_num(&u1,&text,1,max_monthday(tum->start.year,tum->start.month)))
    return false;
  
  tum->start.day = tum->stop.day = u1.val;
  tum->ustart    = tum->ustop    = UNIT_DAY;

  if (u1.len == 1)
    tum->redirect |= true;
    
  if (*text == '\0')
    return true;
  
  if (*text == '-')
    goto tumbler_new_range;
  
  if (*text == '/')
    goto tumbler_new_file;
  
  if (*text != '.')
    return false;
  
  text++;
  if (*text == '\0')
    return false;
  
  /*-----------------------------
  ; parse part
  ;-----------------------------*/
  
  if (!parse_num(&u1,&text,1,INT_MAX))
    return false;
  
  if ((u1.len > 1) && (*u1.txt == '0'))
    tum->redirect |= true;
  
  if (*text == '\0')
    return true;
  
  if (*text != '-')
    return false;
  
  assert(*text == '-');
  goto tumbler_new_range;
  
  /*-----------------------------
  ; parse file
  ;-----------------------------*/
  
tumbler_new_file:

  assert(*text == '/');
  tum->file = true;
  text++;
  
  for (size_t i = 0 ; i < FILENAME_MAX ; i++)
  {
    tum->filename[i] = *text++;

    if (tum->filename[i] == '\0')
      return true;
    if (tum->filename[i] == '/')
      return false;
  }
  
  return false;

tumbler_new_range:

  return false;
}

/************************************************************************/
