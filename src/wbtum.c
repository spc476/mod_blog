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

#define _GNU_SOURCE

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "wbtum.h"
#include "globals.h"

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
  
  assert(pv  != NULL);
  assert(p   != NULL);
  assert(*p  != NULL);
  assert(low <= high);
  
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

static bool check_dates(tumbler__s *tum)
{
  bool start_okay = false;
  bool stop_okay  = false;
  
  assert(tum != NULL);
  
  switch(tum->ustart)
  {
    case UNIT_YEAR:
         start_okay = (btm_cmp_year(&tum->start,&g_blog->first) >= 0)
                   && (btm_cmp_year(&tum->start,&g_blog->last)  <= 0);
         break;

    case UNIT_MONTH:
         start_okay = (btm_cmp_month(&tum->start,&g_blog->first) >= 0)
                   && (btm_cmp_month(&tum->start,&g_blog->last)  <= 0);
         break;
         
    case UNIT_DAY:
         start_okay = (btm_cmp_date(&tum->start,&g_blog->first) >= 0)
                   && (btm_cmp_date(&tum->start,&g_blog->last)  <= 0);
         break;
         
    case UNIT_PART:
         start_okay = (btm_cmp(&tum->start,&g_blog->first) >= 0)
                   && (btm_cmp(&tum->start,&g_blog->last)  <= 0);
         break;
         
    case UNIT_INDEX:
         assert(0);
         return false;
  }

  switch(tum->ustop)
  {
    case UNIT_YEAR:
         stop_okay = (btm_cmp_year(&tum->stop,&g_blog->first) >= 0)
                  && (btm_cmp_year(&tum->stop,&g_blog->last)  <= 0);
         break;

    case UNIT_MONTH:
         stop_okay = (btm_cmp_month(&tum->stop,&g_blog->first) >= 0)
                  && (btm_cmp_month(&tum->stop,&g_blog->last)  <= 0);
         break;
         
    case UNIT_DAY:
         stop_okay = (btm_cmp_date(&tum->stop,&g_blog->first) >= 0)
                  && (btm_cmp_date(&tum->stop,&g_blog->last)  <= 0);
         break;
         
    case UNIT_PART:
         stop_okay = (btm_cmp(&tum->stop,&g_blog->first) >= 0)
                  && (btm_cmp(&tum->stop,&g_blog->last)  <= 0);
         break;
         
    case UNIT_INDEX:
         assert(0);
         return false;
  }
  
  return start_okay && stop_okay;
}

/************************************************************************/

bool tumbler_new(tumbler__s *const tum,const char *text)
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
  bool         part = false;
  
  assert(tum  != NULL);
  assert(text != NULL);
  
  memset(tum,0,sizeof(tumbler__s));
  
  /*-------------------------------------------------
  ; parse year
  ;-------------------------------------------------*/
  
  if (!parse_num(&u1,&text,g_blog->first.year,g_blog->last.year))
    return false;
  
  if (u1.val == g_blog->first.year)
  {
    tum->start.month = g_blog->first.month;
    tum->start.day   = g_blog->first.day;
  }
  else
  {
    tum->start.month = 1;
    tum->start.day   = 1;
  }
  
  tum->ustart      = tum->ustop     = UNIT_YEAR;
  tum->start.year  = tum->stop.year = u1.val;
  tum->start.part  = 1;
  tum->stop.month  = 12;
  tum->stop.day    = 31;
  tum->stop.part   = ENTRY_MAX;
  
  if (*text == '\0')
    return check_dates(tum);
  
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
  
  assert(tum->start.part == 1);
  assert(tum->stop.part  == ENTRY_MAX);
  
  tum->start.month = tum->stop.month = u1.val;
  tum->ustart      = tum->ustop      = UNIT_MONTH;
  tum->stop.day    = max_monthday(tum->start.year,tum->start.month);
  
  if (u1.len == 1)
    tum->redirect |= true;
  
  if (*text == '\0')
    return check_dates(tum);
  
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
    return check_dates(tum);
  
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
  
  if (!parse_num(&u1,&text,1,ENTRY_MAX))
    return false;
  
  tum->start.part = tum->stop.part = u1.val;
  tum->ustart     = tum->ustop     = UNIT_PART;
  
  if ((u1.len > 1) && (*u1.txt == '0'))
    tum->redirect |= true;
  
  if (*text == '\0')
    return check_dates(tum);
  
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
      return check_dates(tum);
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
  
  tum->segments++;
  
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
  
  tum->segments++;
  
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
  
  tum->segments++;
  
  if (*text == '\0')
    goto tumbler_new_calculate;
  
  if (*text != '.')
    return false;

  /*-----------------------------------------------------------------------
  ; the fourth unit, OR the part.  The part will always be stuffed into u4.
  ;------------------------------------------------------------------------*/  

tumbler_new_range_part:
  
  assert(part          == false);
  assert(*text         == '.');
  assert(tum->segments >= 1);
  
  text++;
  
  if (!parse_num(&u4,&text,1,ENTRY_MAX))
    return false;
    
  tum->segments++;
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
  
  if (tum->segments == 4)
  {
    tum->redirect |= (u2.len == 1) 
                  || (u3.len == 1)
                  || ((u4.len > 1) && (*u4.txt == '0'))
                  ;
                  
    tum->stop.year  = u1.val;
    tum->stop.month = u2.val;
    tum->stop.day   = u3.val;
    tum->stop.part  = u4.val;
    tum->ustop      = UNIT_PART;
    return check_dates(tum);
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
  
  else if (tum->segments == 3)
  {
    if (part)
    {
      tum->redirect |= (u1.len == 1)
                    || (u2.len == 1)
                    || ((u4.len > 1) && (*u4.txt == '0'))
                    ;
                    
      tum->stop.month = u1.val;
      tum->stop.day   = u2.val;
      tum->stop.part  = u4.val;
      tum->ustop      = UNIT_PART;
      return check_dates(tum);
    }
    else
    {
      tum->stop.year  = u1.val;
      tum->stop.month = u2.val;
      tum->stop.day   = u3.val;
      tum->stop.part  = ENTRY_MAX;
      return check_dates(tum);
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
  
  else if (tum->segments == 2)
  {
    if (part)
    {
      tum->redirect |= (u1.len == 1)
                    || ((u4.len > 1) && (*u4.txt == '0'))
                    ;
      tum->stop.day  = u1.val;
      tum->stop.part = u4.val;
      tum->ustop     = UNIT_PART;
      return check_dates(tum);
    }
    else
    {
      /*---------------------------------------
      ; check for year/month vs. month/day
      ;--------------------------------------*/
      
      if (u1.val >= g_blog->first.year)
      {
        tum->redirect   |= (u2.len == 1);
        tum->stop.year   = u1.val;
        tum->stop.month  = u2.val;
        tum->stop.day    = max_monthday(tum->stop.year,tum->stop.month);
        tum->stop.part   = ENTRY_MAX;
        tum->ustop       = UNIT_MONTH;
        return check_dates(tum);
      }
      else
      {
        tum->redirect |= (u1.len == 1)
                      || (u2.len == 1)
                      ;
                      
        tum->stop.month = u1.val;
        tum->stop.day   = u2.val;
        tum->stop.part  = ENTRY_MAX;
        tum->ustop      = UNIT_DAY;
        return check_dates(tum);
        
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
           tum->stop.year  = u1.val;
           tum->stop.month = 12;
           tum->stop.day   = 31; /* there are always 31 days in Debtember */
           tum->stop.part  = ENTRY_MAX;
           tum->ustop      = UNIT_YEAR;
           return check_dates(tum);
           
      case UNIT_MONTH:
           tum->redirect   |= (u1.len == 1);
           tum->stop.month  = u1.val;
           tum->stop.day    = max_monthday(tum->stop.year,tum->stop.month);
           tum->stop.part   = ENTRY_MAX;
           tum->ustop       = UNIT_MONTH;
           return check_dates(tum);
           
      case UNIT_DAY:
           tum->redirect  |= (u1.len == 1);
           tum->stop.day   = u1.val;
           tum->stop.part  = ENTRY_MAX;
           tum->ustop      = UNIT_DAY;
           return check_dates(tum);
           
      case UNIT_PART:
           tum->redirect  |= ((u1.len > 1) && (*u1.txt == '0'));
           tum->stop.part  = u1.val;
           tum->ustop      = UNIT_PART;
           return check_dates(tum);
           
      case UNIT_INDEX:
           assert(0);
           return false;
    }
  }
  
  return false;
}

/************************************************************************/

char *tumbler_canonical(const tumbler__s *const tum)
{
  char  start[32];
  char  stop [32];
  char *ret;
  int   rc;
  
  assert(tum != NULL);
  
  if (tum->file)
  {
    rc = asprintf(
                   &ret,
                   "%d/%02d/%02d/%s",
                   tum->start.year,
                   tum->start.month,
                   tum->start.day,
                   tum->filename
                  );
    if (rc == -1)
      return NULL;
    else
      return ret;
  }
  
  switch(tum->ustart)
  {
    case UNIT_YEAR:
         snprintf(start,sizeof(start),"%d",tum->start.year);
         break;
         
    case UNIT_MONTH:
         snprintf(start,sizeof(start),"%d/%02d",tum->start.year,tum->start.month);
         break;
         
    case UNIT_DAY:
         snprintf(start,sizeof(start),"%d/%02d/%02d",tum->start.year,tum->start.month,tum->start.day);
         break;
         
    case UNIT_PART:
         snprintf(start,sizeof(start),"%d/%02d/%02d.%d",tum->start.year,tum->start.month,tum->start.day,tum->start.part);
         break;
         
    case UNIT_INDEX:
         assert(0);
         return NULL;
  }
  
  if (!tum->range)
    return strdup(start);
  
  if (tum->segments == 4)
    snprintf(stop,sizeof(stop),"%d/%02d/%02d.%d",tum->stop.year,tum->stop.month,tum->stop.day,tum->stop.part);
  else if (tum->segments == 3)
  {
    if (tum->ustop == UNIT_PART)
      snprintf(stop,sizeof(stop),"%02d/%02d.%d",tum->stop.month,tum->stop.day,tum->stop.part);
    else
      snprintf(stop,sizeof(stop),"%d/%02d/%02d",tum->stop.year,tum->stop.month,tum->stop.day);
  }
  else if (tum->segments == 2)
  {
    switch(tum->ustop)
    {
      /*-------------------------------------------------------------------
      ; with two segments, we ended up on either a MONTH, DAY or PART. 
      ; There is no way we could end up with a YEAR unless there was a
      ; problem with the code.  Also, there is no way we could get an INDEX
      ; here, again, unless there was a problem with the code.
      ;--------------------------------------------------------------------*/
      
      case UNIT_YEAR:
      case UNIT_INDEX:
           assert(0);
           return NULL;
      
      case UNIT_MONTH:
           snprintf(stop,sizeof(stop),"%d/%02d",tum->stop.year,tum->stop.month);
           break;
           
      case UNIT_DAY:
           snprintf(stop,sizeof(stop),"%02d/%02d",tum->stop.month,tum->stop.day);
           break;
           
      case UNIT_PART:
           snprintf(stop,sizeof(stop),"%02d.%d",tum->stop.day,tum->stop.part);
           break;
    }
  }
  else
  {
    switch(tum->ustop)
    {
      case UNIT_YEAR:
           snprintf(stop,sizeof(stop),"%d",tum->stop.year);
           break;
           
      case UNIT_MONTH:
           snprintf(stop,sizeof(stop),"%02d",tum->stop.month);
           break;
           
      case UNIT_DAY:
           snprintf(stop,sizeof(stop),"%02d",tum->stop.day);
           break;
           
      case UNIT_PART:
           snprintf(stop,sizeof(stop),"%d",tum->stop.part);
           break;
           
      case UNIT_INDEX:
           assert(0);
           return NULL;
    }
  }
  
  rc = asprintf(&ret,"%s-%s",start,stop);
  if (rc == -1)
    return NULL;
  else
    return ret;
}

/************************************************************************/
