
/**************************************************************
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
****************************************************************/

#ifndef CONF_H
#define CONF_H

#include <limits.h>
#include <cgil/pair.h>
#include <cgil/errors.h>

/*****************************************************************
*
* Settable parameters.  Read the description and see if you want
* the option or not.  Then define, undefine and/or change as you
* want.
*
*****************************************************************/

/*---------------------------------------------------------------------
;
; Intially, the program was assumed not-SETUID, so therefore
; any files it needed to create required them to be world writable
; (that included directories as well as files) since files could 
; be created via email, a webpage or locally by the end user.
;
; This might not be desirable so another option is to make both "bp"
; and "addentry" SETUID.  That will prevent the files from being 
; world writable, but does leave a pair of SETUID programs lying around
; that are hopefully secure.
;
; So, for non-SETUID, set this value to 000
; for SETUID operation, set to 022
;
;--------------------------------------------------------------------*/

#define DEFAULT_PERMS	022

/*-----------------------------------------------------
; the following is to support a demo on 2002/7/23.1
; wherein I present a different look and feel to the
; site, *but* links to the parallel site will switch
; back and forth, depending upon which site you are
; viewing.  It's a total hack, but I think it's a 
; pretty cool hack none-the-less.
;-----------------------------------------------------*/
	
#define PARALLEL_HACK

/*-----------------------------------------------------------------------
; Define one of the following:
;	USE_GDBM	- use the GNU Database manipulation routines
;	USE_DB		- use the Berkeley Database manipulation routines
;	USE_HTPASSWD	- use the Apache htpassword format file
;--------------------------------------------------------------------------*/

#undef USE_GDBM
#undef USE_DB
#define USE_HTPASSWD

/*********************************************************************
*
* Non-settable paramters (read:  you best know what you are doing 
* before changing these).  
*
*********************************************************************/

#define AppErr		"BOS"

#ifdef _POSIX_PATH_MAX
#  define FILENAME_LEN	_POSIX_PATH_MAX
#else
#  define FILENAME_LEN 255
#endif

#define BLOGINIT		(ERR_APP + 0)
#define BLOGDAYREAD		(ERR_APP + 1)
#define BLOGDAYWRITE		(ERR_APP + 2)
#define BLOGENTRYADD		(ERR_APP + 3)
#define BLOGENTRYREAD		(ERR_APP + 4)
#define BLOGENTRYWRITE		(ERR_APP + 5)

#define APPERR_OPEN		(ERR_APP + 1)
#define APPERR_WRITE		(ERR_APP + 2)
#define APPERR_CONF		(ERR_APP + 3)
#define APPERR_CONFOPEN		(ERR_APP + 4)
#define APPERR_BLOGINIT		(ERR_APP + 5)
#define APPERR_GENPAGE		(ERR_APP + 6)
#define APPERR_NOBACKEND	(ERR_APP + 7)

#define CHUNKPROCESS		(ERR_APP + 6)
#define CHUNKERR_OPEN		(ERR_APP + 3)

#define GLOBALINIT		(ERR_APP + 10)

#define DISPLAYENTRYSTRING	(ERR_APP + 10)

#define REQUESTERR_BADDAY	(ERR_APP + 10)
#define REQUESTERR_BADMONTH	(ERR_APP + 11)
#define REQUESTERR_BADYEAR	(ERR_APP + 12)
#define REQUESTERR_UNITS	(ERR_APP + 13)

#define ADDENTRY_MAIN		(ERR_APP + 14)

#endif

