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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <cgilib/memory.h>
#include <cgilib/ddt.h>
#include <cgilib/nodelist.h>
#include <cgilib/errors.h>
#include <cgilib/types.h>
#include <cgilib/util.h>

#include "wbtum.h"

/**************************************************************************/

typedef struct fpreturn
{
  struct fpreturn (*state)(Tumbler,TumblerUnit *,char **);
} State;

static TumblerUnit	tumblerunit_new		(enum ttypes);
static State		state_a			(Tumbler,TumblerUnit *,char **);
static State		state_b			(Tumbler,TumblerUnit *,char **);
static State		state_c			(Tumbler,TumblerUnit *,char **);
static State		state_d			(Tumbler,TumblerUnit *,char **);
static State		state_e			(Tumbler,TumblerUnit *,char **);
static int 		tumbler_normalize	(Tumbler t);

/*********************************************************************/

int (TumblerNew)(Tumbler *pt,char **pstr)
{
  Tumbler         t;
  TumblerUnit     tu;
  struct fpreturn current;
  
  ddt(pt    != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);
  
  t = MemAlloc(sizeof(struct tumbler));
  ListInit(&t->units);
  t->pairs          = 0;
  t->flags.file     = FALSE;
  t->flags.redirect = FALSE;
  t->flags.error    = FALSE;
  
  tu            = tumblerunit_new(TUMBLER_SINGLE);
  current.state = state_a;
  
  while(current.state)
    current = (*current.state)(t,&tu,pstr);

  tumbler_normalize(t);
  
  if (t->flags.error)
  {
    TumblerFree(&t);
    return(ERR_ERR);
  }
  
  *pt = t;
  return(ERR_OKAY);  
}

/************************************************************************/

int (TumblerFree)(Tumbler *pt)
{
  Tumbler     t;
  TumblerUnit tu;
  
  ddt(pt  != NULL);
  ddt(*pt != NULL);
  
  t = *pt;
  
  for (
        tu = (TumblerUnit)ListRemHead(&t->units);
        NodeValid(&tu->node);
        tu = (TumblerUnit)ListRemHead(&t->units)
      )
  {
    if (tu->file) MemFree(tu->file);
    MemFree(tu);
  }
  MemFree(t);
  *pt = NULL;
  return(ERR_OKAY);
}

/**********************************************************************/
        
static TumblerUnit tumblerunit_new(enum ttypes type)
{
  TumblerUnit tu;
  
  ddt(
       (type == TUMBLER_SINGLE)
       || (type == TUMBLER_RANGE)
     );
     
  tu        = MemAlloc(sizeof(struct tumunit));
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
  
  ddt(t     != NULL);
  ddt(ptu   != NULL);
  ddt(*ptu  != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);
  
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
        t->flags.error = TRUE;
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
#if 0 
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
#ifdef OHNO
      t->flags.redirect = TRUE;
#endif
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
  
    t->flags.error = TRUE;
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
  
  ddt(t     != NULL);
  ddt(ptu   != NULL);
  ddt(*ptu  != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);
  
  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    tu->entry[MONTH] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = TRUE;
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
  
  if (*s == '/')
  {
    *pstr = ++s;
    *ptu  = tu;
    next.state = state_c;
    return(next);
  }
  
  if ((*s == '.') || (*s == ':'))
  {
#ifdef OHNO
    t->flags.redirect = TRUE;
#endif
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
  
#if 0

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

  t->flags.error = TRUE;
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
  
  ddt(t     != NULL);
  ddt(ptu   != NULL);
  ddt(*ptu  != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);
  
  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    tu->entry[DAY] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = TRUE;
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
  
  if (*s == '/')
  {
    *pstr      = ++s;
    *ptu       = tu;		/* t->pairs != 0 ? error? */
    next.state = state_d;
    return(next);
  }
  
  if (*s == ':')
  {
#ifdef OHNO
    t->flags.redirect = TRUE;
#endif
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
  
#if 0

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

  t->flags.error = TRUE;
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
  
  ddt(t     != NULL);
  ddt(ptu   != NULL);
  ddt(*ptu  != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);

  next.state = NULL;
  
  if (t->pairs > 0)
  {
    t->flags.error = TRUE;
    return(next);
  }

  sfn = strlen(*pstr);
  if (sfn)
  {
    (*ptu)->file   = dup_string(*pstr);
    t->flags.file  = TRUE;
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

  ddt(t     != NULL);
  ddt(ptu   != NULL);
  ddt(*ptu  != NULL);
  ddt(pstr  != NULL);
  ddt(*pstr != NULL);

  s  = *pstr;
  tu = *ptu;

  if (isdigit(*s))
  {
    tu->entry[PART] = strtoul(s,&p,10);
    tu->size++;
    if (s == p)
    {
      t->flags.error = TRUE;
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
#if 0 
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
  t->flags.error = TRUE;
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
  
  ddt(t != NULL);
  
  if (t->flags.file) return(ERR_OKAY);
  /*if (t->pairs == 1) return(ERR_OKAY);*/
  
  base    = last = (TumblerUnit)ListGetHead(&t->units);
  if ((base->type == TUMBLER_SINGLE) && (t->pairs == 1)) return(ERR_OKAY);
  current = (TumblerUnit)NodeNext(&last->node);
  ddt(NodeValid(&current->node));
  
  while(NodeValid(&current->node))
  {
    if (current->size >= base->size)
      base = current;
    
    /*------------------------------------------------------------
    ; if the number of segments are the same, we're done.
    ; If the number of segments in the base are larger, then the
    ; current is a subset of the base, so move the indicies down and
    ; copy down the major portion.  
    ; If the current side is larger, then we're already done.
    ;-------------------------------------------------------------*/
                                
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
      ddt(base != last);		/* make sure we do good testing */
      NodeRemove(&current->node);
      if (current->file) MemFree(current->file);
      MemFree(current);
      t->flags.redirect = TRUE;
      current = last;
    }
    
    last    = current;
    current = (TumblerUnit)NodeNext(&current->node);
  }
  return(ERR_OKAY);
}

/*********************************************************************/

