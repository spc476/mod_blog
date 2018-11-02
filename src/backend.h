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

#ifndef I_5B8ED10A_F8F4_5F83_A7EA_CD76EE7A05D8
#define I_5B8ED10A_F8F4_5F83_A7EA_CD76EE7A05D8

typedef struct template
{
  char const  *template;
  char const  *file;
  size_t       items;
  int        (*pagegen)(struct template const *,FILE *,Blog *);
  bool         reverse;
  bool         fullurl;
} template__t;

typedef struct aflink
{
  char const *proto;
  size_t      psize;
  char const *format;
} aflink__t;

struct callback_data
{
  List        list;
  struct btm  last;     /* timestamp of previous entry */
  BlogEntry  *entry;    /* current entry being processed */
  FILE       *ad;       /* file containing ad */
  char       *adtag;
};

/************************************************/

extern int  generate_pages (void);
extern int  pagegen_items  (template__t const *,FILE *,Blog *);
extern int  pagegen_days   (template__t const *,FILE *,Blog *);
extern int  tumbler_page   (FILE *,tumbler__s *);
extern void generic_cb     (char const *,FILE *,void *);

#endif
