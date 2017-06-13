/****************************************************************
*
* Copyright 2006 by Sean Conner.  All Rights Reserved.
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

#ifndef I_F4A408FA_A630_59FB_81C6_846355DD0AFB
#define I_F4A408FA_A630_59FB_81C6_846355DD0AFB

/*********************************************************************/

typedef struct mystring
{
  size_t      s;
  char const *d;
} String;

/*********************************************************************/

extern String *tag_split  (size_t *const,char const *);
extern char   *fromstring (String const);
extern size_t  fcopy      (FILE *const restrict,FILE *const restrict);

#endif
