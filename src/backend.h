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
  const char  *file;
  size_t       items;
  int        (*pagegen)(const struct template *const,FILE *const,Blog const);
  bool         reverse;
  bool         fullurl;
} template__t;

typedef struct aflink
{
  const char *proto;
  size_t      psize;
  const char *format;
} aflink__t;

/************************************************/

extern int generate_pages	(void);
extern int pagegen_items	(const template__t *const,FILE *const,Blog const);
extern int pagegen_days		(const template__t *const,FILE *const,Blog const);
extern int tumbler_page		(FILE *,tumbler__s *);

#endif
