
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

#ifndef BLOG_ARRAY
#define BLOG_ARRAY

typedef struct array
{
  size_t  currsize;	/* size of current allocation 	*/
  size_t  index;	/* next unused slot		*/
  size_t  size;		/* size of each element 	*/
  void   *data;
} Array;

/******************************************************************/

Array		 (ArrayNew)	(size_t);
Array		 (ArrayAdd)	(Array,void *);
size_t		 (ArraySize)	(const Array);
void		*(ArrayData)	(const Array);
int		 (ArrayFree)	(Array);

#endif

