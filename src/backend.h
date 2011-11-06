/***********************************************
*
* Copyright 2011 by Sean Conner.  All Rights Reserved.
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
************************************************/

#ifndef BACKEND_H
#define BACKEND_H

typedef struct template
{
  const char  *template;
  size_t       items;
  int        (*pagegen)(const struct template *const restrict,FILE *const restrict,const struct btm *const restrict);
  bool         reverse;
  bool         fullurl;
} template__t;

/************************************************/

int page_generation	(Request);
int pagegen_items	(const template__t *const restrict,FILE *const restrict,const struct btm *const restrict);
int pagegen_days	(const template__t *const restrict,FILE *const restrict,const struct btm *const restrict);

#endif