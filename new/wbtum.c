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
  /*----------------------------------------------------------------------
  ; all variables are defined here, even though u2, u3, u4, segments and
  ; part aren't used until tumbler_new_range.  It appears that defining them
  ; there causes a compiler error.  It's either a bug in the version of GCC
  ; I'm using, or you can't declare variables after a goto target label.
  ;
  ; I'm not sure what the case is here, so I'm moving them up here.
  ;----------------------------------------------------------------------*/
  
  struct value u1;
  struct value u2;
  struct value u3;
  struct value u4;
  int          segments = 0;
  bool         part     = false;
  
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
  
  tum->start.part = tum->stop.part = u1.val;
  tum->ustart     = tum->ustop     = UNIT_PART;
  
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
  
  /*--------------------------------------
  ; parse the range portion of the tumbler.  Since the number of segments in
  ; this portion is variable, we parse first without reguard to the actual
  ; ranges, keep track of the number of segments found, then after we're
  ; done with the parsing, we figure everything out.
  ;---------------------------------------*/
  
tumbler_new_range:

  assert(*text == '-');
  tum->range = true;
  text++;
  
  /*--------------------
  ; first unit
  ;---------------------*/
  
  if (!parse_num(&u1,&text,1,INT_MAX))
    return false;
  
  segments++;
  
  if (*text == '\0')
    goto tumbler_new_calculate;
  
  if (*text == '.')
    goto tumbler_new_range_part;
  
  if (*text != '/')
    return false;
  
  text++;
  if (*text == '\0')
  {
    tum->redirect |= true;
    goto tumbler_new_calculate;
  }
  
  /*------------------------------
  ; second unit
  ;------------------------------*/
  
  if (!parse_num(&u2,&text,1,INT_MAX))
    return false;
  
  segments++;
  
  if (*text == '\0')
    goto tumbler_new_calculate;
  
  if (*text == '.')
    goto tumbler_new_range_part;
  
  if (*text != '/')
    return false;
  
  text++;
  if (*text == '\0')
  {
    tum->redirect |= true;
    goto tumbler_new_calculate;
  }
  
  /*----------------------------------
  ; third unit
  ;-----------------------------------*/
  
  if (!parse_num(&u3,&text,1,INT_MAX))
    return false;
  
  segments++;
  
  if (*text == '\0')
    goto tumbler_new_calculate;
  
  if (*text != '.')
    return false;

  /*-----------------------------------------------------------------------
  ; the fourth unit, OR the part.  The part will always be stuffed into u4.
  ;------------------------------------------------------------------------*/  

tumbler_new_range_part:
  
  assert(part     == false);
  assert(*text    == '.');
  assert(segments >= 1);
  
  text++;
  
  if (!parse_num(&u4,&text,1,INT_MAX))
    return false;
    
  segments++;
  part = true;
  
  if ((u4.len > 1) && (*u4.txt == '0'))
    tum->redirect |= true;
  
  if (*text != '\0')
    return false;
  
  /*----------------------------------------------------------------------
  ; Now figure everything out.  If we have all four segments, then this part
  ; is easy---we fill out every part of the range portion with our values. 
  ; All values have already been checked against the lower bound of 1, so we
  ; only have to check against the upper limit for the most part.
  ;
  ; Here is also were we check for leading or missing zeros, and set the
  ; redirect flag appropriately.  The month and day require a leading zero,
  ; while the part does not.
  ;------------------------------------------------------------------------*/

tumbler_new_calculate:
  
  if (segments == 4)
  {
    if ((u1.val < 1999) || (u1.val > 2200))
      return false;
    
    if (u2.val > 12)
      return false;
    
    if (u3.val > max_monthday(u1.val,u2.val))
      return false;
    
    tum->redirect |= (u2.len == 1) 
                  || (u3.len == 1)
                  || ((u4.len > 1) && (*u4.txt == '0'))
                  ;
                  
    tum->stop.year  = u1.val;
    tum->stop.month = u2.val;
    tum->stop.day   = u3.val;
    tum->stop.part  = u4.val;
    tum->ustop      = UNIT_PART;
    return true;
  }
  
  /*---------------------------------------------------------------
  ; If we have three segments, there are two cases to consider:
  ;
  ;	2000/02/03-2004/05/06
  ;
  ; or
  ;
  ;	2000/02/03-04/05.6
  ;
  ; The first is when part is false.  This is a fairly easy case to handle.
  ;-----------------------------------------------------------------------*/
  
  else if (segments == 3)
  {
    if (part)
    {
      if (u1.val > 12)
        return false;
      if (u2.val > max_monthday(tum->stop.year,u1.val))
        return false;
      
      tum->redirect |= (u1.len == 1)
                    || (u2.len == 1)
                    || ((u4.len > 1) && (*u4.txt == '0'))
                    ;
                    
      tum->stop.month = u1.val;
      tum->stop.day   = u2.val;
      tum->stop.part  = u4.val;
      tum->ustop      = UNIT_PART;
      return true;
    }
    else
    {
      if ((u1.val < 1999) || (u1.val > 2200))
        return false;
      if (u2.val > 12)
        return false;
      if (u3.val > max_monthday(u1.val,u2.val))
        return false;
      
      tum->stop.year  = u1.val;
      tum->stop.month = u2.val;
      tum->stop.day   = u3.val;
      tum->stop.part  = INT_MAX;
      return true;
    }
  }
  
  /*------------------------------------------------------------------
  ; We have two segments.  If there is a part, then it's easy:
  ;
  ;	2000/02/03-04.1
  ;
  ; The next few cases not so:
  ;
  ;	2000/02/03-2004/05
  ;	2000/02/03-04/05
  ;
  ; We use some huristics to distinquish between the two.  This means we
  ; can't specify years before 13 AD, but I doubt that will be an issue any
  ; time soon.
  ;------------------------------------------------------------------------*/
  
  else if (segments == 2)
  {
    if (part)
    {
      if (u1.val > max_monthday(tum->stop.year,tum->stop.month))
        return false;
      
      tum->redirect |= (u1.len == 1)
                    || ((u4.len > 1) && (*u4.txt == '0'))
                    ;
      tum->stop.day  = u1.val;
      tum->stop.part = u4.val;
      tum->ustop     = UNIT_PART;
      return true;
    }
    else
    {
      if (u1.val >= 1999)
      {
        if (u1.val > 2200)
          return false;
        if (u2.val > 12)
          return false;
        
        tum->redirect   |= (u2.len == 1);
        tum->stop.year   = u1.val;
        tum->stop.month  = u2.val;
        tum->stop.day    = max_monthday(tum->stop.year,tum->stop.month);
        tum->stop.part   = INT_MAX;
        tum->ustop       = UNIT_MONTH;
        return true;
      }
      else
      {
        if (u1.val > 12)
          return false;
        if (u2.val > max_monthday(tum->stop.year,u1.val))
          return false;
          
        tum->redirect |= (u1.len == 1)
                      || (u2.len == 1)
                      ;
                      
        tum->stop.month = u1.val;
        tum->stop.day   = u2.val;
        tum->stop.part  = INT_MAX;
        tum->ustop      = UNIT_DAY;
        return true;
        
      }
    }
  }
  
  /*------------------------------------------------------------------
  ; Final case---one segment.  We use the unit defined in the starting
  ; portion to determine what type of segment this is.
  ;-------------------------------------------------------------------*/
  
  else
  {
    switch(tum->ustart)
    {
      case UNIT_YEAR:
           if ((u1.val < 1999) || (u1.val > 2200))
             return false;
             
           tum->stop.year  = u1.val;
           tum->stop.month = 12;
           tum->stop.day   = 31; /* there are always 31 days in Debtember */
           tum->stop.part  = INT_MAX;
           tum->ustop      = UNIT_YEAR;
           return true;
           
      case UNIT_MONTH:
           if (u1.val > 12)
             return false;
           
           tum->redirect   |= (u1.len == 1);
           tum->stop.month  = u1.val;
           tum->stop.day    = max_monthday(tum->stop.year,tum->stop.month);
           tum->stop.part   = INT_MAX;
           tum->ustop       = UNIT_MONTH;
           return true;
           
      case UNIT_DAY:
           if (u1.val > max_monthday(tum->stop.year,tum->stop.month))
             return false;
           
           tum->redirect  |= (u1.len == 1);
           tum->stop.day   = u1.val;
           tum->stop.part  = INT_MAX;
           tum->ustop      = UNIT_DAY;
           return true;
           
      case UNIT_PART:
           tum->redirect  |= ((u1.len > 1) && (*u1.txt == '0'));
           tum->stop.part  = u1.val;
           tum->ustop      = UNIT_PART;
           return true;
           
      case UNIT_INDEX:
           assert(0);
           return false;
    }
  }
  
  return false;
}

/************************************************************************/
