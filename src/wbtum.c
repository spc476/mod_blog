/*************************************************************************
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
*************************************************************************/

/********************************************************************
*
* There are a few comments in here about canonical URLs.  Canonical URLs
* based upon tumblers are of the form:
*
*	YYYY/MM/DD
*	
* The form:
*
*	YYYY/M/D
*
* are considered non-canonical and are marked for a later perm. redirect. 
* The rational for this is the Google duplicate-content problem.  Since the
* two URLs of:
*
*	1999/3/4
*	1999/03/04
*
* referr to the same entry, Google will see it as duplicate content and
* penalize the Page Ranking by a bit.  By detecting this and offering a
* (permanent) redirect, we can canonalize the URLs and avoid this problem.
*
************************************************************************/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

#include <cgilib6/nodelist.h>
#include <cgilib6/util.h>

#include "conf.h"
#include "wbtum.h"

/**************************************************************************/

typedef struct fpreturn
{
  struct fpreturn (*state)(Tumbler,TumblerUnit *,char **);
} State;

static void		tumblerunit_canonize	(char **,TumblerUnit);
static TumblerUnit	tumblerunit_new		(enum ttypes);
static State		state_a			(Tumbler,TumblerUnit *,char **);
static State		state_b			(Tumbler,TumblerUnit *,char **);
static State		state_c			(Tumbler,TumblerUnit *,char **);
static State		state_d			(Tumbler,TumblerUnit *,char **);
static State		state_e			(Tumbler,TumblerUnit *,char **);
static int 		tumbler_normalize	(Tumbler t);

/*********************************************************************/

int TumblerNew(Tumbler *pt,char **pstr)
{
  Tumbler         t;
  TumblerUnit     tu;
  struct fpreturn current;
  
  assert(pt    != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);
  
  t = malloc(sizeof(struct tumbler));
  ListInit(&t->units);
  t->pairs          = 0;
  t->flags.file     = false;
  t->flags.redirect = false;
  t->flags.error    = false;
  
  tu            = tumblerunit_new(TUMBLER_SINGLE);
  current.state = state_a;
  
  while(current.state)
    current = (*current.state)(t,&tu,pstr);

  tumbler_normalize(t);
  
  if (t->flags.error)
  {
    TumblerFree(&t);
    return(-1);
  }

  /*------------------------------------------
  ; I need to rethink how this is done, but for
  ; now, we don't redirect on tumbler ranges
  ;--------------------------------------------*/
  
  tu = (TumblerUnit)ListGetHead(&t->units);
  if (tu->type == TUMBLER_RANGE)
    t->flags.redirect = false;
 
  *pt = t;
  return(0);  
}

/************************************************************************/

char *TumblerCanonical(Tumbler t)
{
  TumblerUnit  tu;
  char        *text = NULL;

  tu  = (TumblerUnit)ListGetHead(&t->units);

  if (tu->type == TUMBLER_SINGLE)
    tumblerunit_canonize(&text,tu);
  else if (tu->type == TUMBLER_RANGE)
    assert(0);
  else
    assert(0);

  return(text);
}

/************************************************************************/

static void tumblerunit_canonize(char **ptext,TumblerUnit tu)
{
  assert(ptext != NULL);
  assert(tu    != NULL);
  
  switch(tu->size)
  {
    case 1:
         asprintf(ptext,"%d",tu->entry[0]);
         break;
    case 2:
         asprintf(ptext,"%d/%02d",tu->entry[0],tu->entry[1]);
         break;
    case 3:
         if (tu->file)
           asprintf(ptext,"%d/%02d/%02d/%s",tu->entry[0],tu->entry[1],tu->entry[2],tu->file);
         else
           asprintf(ptext,"%d/%02d/%02d",tu->entry[0],tu->entry[1],tu->entry[2]);
         break;
    case 4:
         asprintf(ptext,"%d/%02d/%02d.%d",tu->entry[0],tu->entry[1],tu->entry[2],tu->entry[3]);
         break;
    case 0:
    default:
         assert(0);
         break;         
  }  
}

/************************************************************************/

int TumblerFree(Tumbler *pt)
{
  Tumbler     t;
  TumblerUnit tu;
  
  assert(pt  != NULL);
  assert(*pt != NULL);
  
  t = *pt;
  
  for (
        tu = (TumblerUnit)ListRemHead(&t->units);
        NodeValid(&tu->node);
        tu = (TumblerUnit)ListRemHead(&t->units)
      )
  {
    if (tu->file) free(tu->file);
    free(tu);
  }
  free(t);
  *pt = NULL;
  return(0);
}

/**********************************************************************/
        
static TumblerUnit tumblerunit_new(enum ttypes type)
{
  TumblerUnit tu;
  
  assert(
       (type == TUMBLER_SINGLE)
       || (type == TUMBLER_RANGE)
     );
     
  tu        = malloc(sizeof(struct tumunit));
  tu->type  = type;
  tu->file  = NULL;
  tu->size  = 0;
  memset(tu->entry,0,sizeof(tu->entry));
  return(tu);
}

/***********************************************************************/

static struct fpreturn state_a(Tumbler t,TumblerUnit *ptu,char **pstr)
{
  char            *s;
  char            *p;
  TumblerUnit      tu;
  struct fpreturn  next;
  
  assert(t     != NULL);
  assert(ptu   != NULL);
  assert(*ptu  != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);
  
  s  = *pstr;
  tu = *ptu;
  
  while(1)
  {
    if (isdigit(*s))
    {
      tu->entry[YEAR] = strtoul(s,&p,10);
      tu->size++;
      if (s == p)
      {
        t->flags.error = true;
        next.state     = NULL;
        return(next);
      }
      s = p;
    }
  
    if (*s == '\0')
    {
      ListAddTail(&t->units,&tu->node);
      t->pairs++;
      *pstr      = s;
      *ptu       = tu;
      next.state = NULL;
      return(next);
    }
#ifdef FEATURE_MULTI_TUMBLER
    if (*s == ',')
    {
      ListAddTail(&t->units,&tu->node);
      t->pairs++;
      tu = tumblerunit_new(TUMBLER_SINGLE);
      s++;
      continue;
    }
#endif 
    if (*s == '-')
    {
      tu->type = TUMBLER_RANGE;
      ListAddTail(&t->units,&tu->node);
      tu = tumblerunit_new(TUMBLER_SINGLE);
      s++;
      continue;
    }
  
    if ((*s == '.') || (*s == ':'))
    {
      t->flags.redirect = true;
      *pstr      = ++s;
      *ptu       = tu;
      next.state = state_b;
      return(next);
    }
  
    if (*s == '/')
    {
      *pstr      = ++s;
      *ptu       = tu;
      next.state = state_b;
      return(next);
    }
  
    t->flags.error = true;
    *pstr          = s;
    *ptu           = tu;
    next.state     = NULL;
    return(next);
  }
}

/************************************************************************/

static State state_b(Tumbler t,TumblerUnit *ptu,char **pstr)
{
  struct fpreturn  next;
  TumblerUnit      tu;
  char            *s;
  char            *p;
  
  assert(t     != NULL);
  assert(ptu   != NULL);
  assert(*ptu  != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);
  
  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    tu->entry[MONTH] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = true;
      next.state     = NULL;
      return(next);
    }

    /*-------------------------------------------
    ; Canonical URLs contain a two-digit
    ; month.  If not, mark it as a non-canonical
    ;-------------------------------------------*/
        
    if (p != (s + 2))
      t->flags.redirect = true;

    s = p;
  }
  
  if (*s == '\0')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = s;
    *ptu       = tu;
    next.state = NULL;
    return(next);
  }
  
  if (*s == '/')
  {
    *pstr = ++s;
    *ptu  = tu;
    next.state = state_c;
    return(next);
  }
  
  if ((*s == '.') || (*s == ':'))
  {
    t->flags.redirect = true;
    *pstr      = ++s;
    *ptu       = tu;
    next.state = state_c;
    return(next);
  }
  
  if (*s == '-')
  {
    tu->type = TUMBLER_RANGE;
    ListAddTail(&t->units,&tu->node);
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
  
#ifdef FEATURE_MULTI_TUMBLER

  /*----------------------------------------
  ; for now, skip handling multiple tumblers
  ;-----------------------------------------*/

  if (*s == ',')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
#endif

  t->flags.error = true;
  *pstr          = s;
  *ptu           = tu;
  next.state     = NULL;
  return(next);
}

/************************************************************************/

static State state_c(Tumbler t,TumblerUnit *ptu,char **pstr)
{
  struct fpreturn  next;
  TumblerUnit      tu;
  char            *s;
  char            *p;
  
  assert(t     != NULL);
  assert(ptu   != NULL);
  assert(*ptu  != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);
  
  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    tu->entry[DAY] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = true;
      next.state     = NULL;
      return(next);
    }

    /*----------------------------------------------
    ; Canonical URLs contain a two-digit day.  If not,
    ; mark as such.
    ;-----------------------------------------------*/
    
    if (p != (s + 2))
      t->flags.redirect = true;

    s = p;
  }
  
  if (*s == '\0')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = s;
    *ptu       = tu;
    next.state = NULL;
    return(next);
  }
  
  if (*s == '/')
  {
    *pstr      = ++s;
    *ptu       = tu;		/* t->pairs != 0 ? error? */
    next.state = state_d;
    return(next);
  }
  
  if (*s == ':')
  {
    t->flags.redirect = true;
    *pstr      = ++s;
    *ptu       = tu;
    next.state = state_e;
    return(next);
  }
  
  if (*s == '.')
  {
    *pstr      = ++s;
    *ptu       = tu;
    next.state = state_e;
    return(next);
  }
  
  if (*s == '-')
  {
    tu->type = TUMBLER_RANGE;
    ListAddTail(&t->units,&tu->node);
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
  
#ifdef FEATURE_MULTI_TUMBLER 

  /*----------------------------------
  ; for now, skip handling multiple
  ; tumblers.
  ;----------------------------------*/

  if (*s == ',')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
#endif

  t->flags.error = true;
  *pstr          = s;
  *ptu           = tu;
  next.state     = NULL;
  return(next);
}

/*************************************************************************/

static State state_d(Tumbler t,TumblerUnit *ptu,char **pstr)
{
  struct fpreturn next;
  size_t          sfn;
  
  assert(t     != NULL);
  assert(ptu   != NULL);
  assert(*ptu  != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);

  next.state = NULL;
  
  if (t->pairs > 0)
  {
    t->flags.error = true;
    return(next);
  }

  sfn = strlen(*pstr);
  if (sfn)
  {
    (*ptu)->file   = strdup(*pstr);
    t->flags.file  = true;
    *pstr         += sfn;
  }
  
  ListAddTail(&t->units,&(*ptu)->node);
  t->pairs++;
  return(next);
}

/**********************************************************************/

static State state_e(Tumbler t,TumblerUnit *ptu,char **pstr)
{
  struct fpreturn  next;
  TumblerUnit      tu;
  char            *s;
  char            *p;

  assert(t     != NULL);
  assert(ptu   != NULL);
  assert(*ptu  != NULL);
  assert(pstr  != NULL);
  assert(*pstr != NULL);

  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    /*---------------------------------------------------------------------
    ; Canonical URLs do not have leading zeros in the part section of the
    ; tumbler.  If so, mark it as non-canonical.
    ;--------------------------------------------------------------------*/
    
    if (*s == '0')
      t->flags.redirect = true;

    tu->entry[PART] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = true;
      next.state     = NULL;
      return(next);
    }
    s = p;
  }
  
  if (*s == '\0')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = s;
    *ptu       = tu;
    next.state = NULL;
    return(next);
  }
  
  if (*s == '-')
  {
    tu->type = TUMBLER_RANGE;
    ListAddTail(&t->units,&tu->node);
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
#ifdef FEATURE_MULTI_TUMBLER
  if (*s == ',')
  {
    ListAddTail(&t->units,&tu->node);
    t->pairs++;
    *pstr      = ++s;
    *ptu       = tumblerunit_new(TUMBLER_SINGLE);
    next.state = state_a;
    return(next);
  }
#endif 
  t->flags.error = true;
  *pstr          = s;
  *ptu           = tu;
  next.state     = NULL;
  return(next);
}

/*********************************************************************/

static int tumbler_normalize(Tumbler t)
{
  TumblerUnit base;
  TumblerUnit last;
  TumblerUnit current;
  
  assert(t != NULL);
  
  if (t->flags.file) return(0);
  
  base    = last = (TumblerUnit)ListGetHead(&t->units);
  if ((base->type == TUMBLER_SINGLE) && (t->pairs == 1)) return(0);
  current = (TumblerUnit)NodeNext(&last->node);

  while(NodeValid(&current->node))
  {
    if (current->size >= base->size)
      base = current;
    
    /*----------------------------------------------------------------------
    ; if the number of segments are the same, we're done.  If the number of
    ; segments in the base are larger, then the current is a subset of the
    ; base, so move the indicies down and copy down the major portion.  If
    ; the current side is larger, then we're already done.
    ;---------------------------------------------------------------------*/
    
    if (base->size > current->size)
    {
      size_t diff;
      size_t i;
      
      diff = base->size - current->size;
      memmove(&current->entry[diff],&current->entry[0],current->size * sizeof(int));
      for (i = 0 ; i < diff ; i++)
        current->entry[i] = base->entry[i];
      current->size = base->size;
    }
    
    /*-------------------------------------------------------------
    ; two ranges in a row is not allowed.  If there are two ranges
    ; in a row, collapse the two together.
    ;-------------------------------------------------------------*/
    
    if ((last->type == TUMBLER_RANGE) && (current->type == TUMBLER_RANGE))
    {
      assert(base != last);		/* make sure we do good testing */
      NodeRemove(&current->node);
      if (current->file) free(current->file);
      free(current);
      t->flags.redirect = true;
      current = last;
    }
    
    last    = current;
    current = (TumblerUnit)NodeNext(&current->node);
  }
  return(0);
}

/*********************************************************************/

