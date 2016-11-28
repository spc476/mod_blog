/************************************************************************
*
* Copyright 2005 by Sean Conner.  All Rights Reserved.
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

#ifndef I_923B1CED_AA68_5BFE_A50A_C27EEE5A2EE8
#define I_923B1CED_AA68_5BFE_A50A_C27EEE5A2EE8

#include <cgilib6/chunk.h>

#include "frontend.h"

#define DB_BLOCK	1024

	/*-----------------------------------
	; Informational Codes
	;----------------------------------*/

typedef enum http__e
{
  HTTP_CONTINUE		= 100,		/* 1.1	*/
  HTTP_SWITCHPROTO	= 101,		/* 1.1	*/
  HTTP_PROCESSING       = 102,		/* RFC-2518 */
  HTTP_CHECKPOINT       = 103,		/* U */

	/*--------------------------------------
	; Successful Codes
	;--------------------------------------*/

  HTTP_OKAY		= 200,		/* 1.0	*/
  HTTP_CREATED		= 201,		/* 1.0	*/
  HTTP_ACCEPTED		= 202,		/* 1.0	*/
  HTTP_NONAUTH		= 203,		/* 1.1	*/
  HTTP_NOCONTENT	= 204,		/* 1.0	*/
  HTTP_RESETCONTENT	= 205,		/* 1.1	*/
  HTTP_PARTIALCONTENT	= 206,		/* RFC-7233 */
  HTTP_MULTISTATUS      = 207,		/* RFC-4918 */
  HTTP_ALREADYREPORTED  = 208,		/* RFC-5842 */
  HTTP_IMUSED           = 226,		/* RFC-3229 */

	/*----------------------------------------
	; Redirection Codes
	;----------------------------------------*/

  HTTP_300		= 300,
  HTTP_MULTICHOICE	= 300,		/* 1.1	*/
  HTTP_MOVEPERM		= 301,		/* 1.0	*/
  HTTP_MOVETEMP		= 302,		/* 1.0	*/
  HTTP_SEEOTHER		= 303,		/* 1.1	*/
  HTTP_NOTMODIFIED	= 304,		/* 1.0	*/
  HTTP_USEPROXY		= 305,		/* 1.1	*/
  HTTP_SWITCHPROXY      = 306,		/* 1.1  */
  HTTP_MOVETEMP_M       = 307,		/* 1.1  */
  HTTP_MOVEPERM_M       = 308,		/* RFC-7538 */

	/*-----------------------------------------
	; Client Error
	;----------------------------------------*/

  HTTP_BADREQ		= 400,		/* 1.0	*/
  HTTP_UNAUTHORIZED	= 401,		/* 1.0	*/
  HTTP_PAYMENTREQ	= 402,		/* 1.1	*/
  HTTP_FORBIDDEN	= 403,		/* 1.0	*/
  HTTP_NOTFOUND		= 404,		/* 1.0	*/
  HTTP_METHODNOTALLOWED	= 405,		/* 1.1	*/
  HTTP_NOTACCEPTABLE	= 406,		/* 1.1	*/
  HTTP_PROXYAUTHREQ	= 407,		/* 1.1	*/
  HTTP_TIMEOUT		= 408,		/* 1.1	*/
  HTTP_CONFLICT		= 409,		/* 1.1	*/
  HTTP_GONE		= 410,		/* 1.1	*/
  HTTP_LENGTHREQ	= 411,		/* 1.1	*/
  HTTP_PRECONDITION	= 412,		/* 1.1	*/
  HTTP_TOOLARGE		= 413,		/* 1.1	*/
  HTTP_URITOOLONG	= 414,		/* 1.1	*/
  HTTP_MEDIATYPE	= 415,		/* 1.1	*/
  HTTP_RANGEERROR	= 416,		/* 1.1	*/
  HTTP_EXPECTATION	= 417,		/* 1.1	*/
  HTTP_IM_A_TEA_POT     = 418,		/* RFC-2324 */
  HTTP_METHODFAILURE    = 420,		/* U */
  HTTP_MISDIRECT        = 421,		/* RFC-7540 */
  HTTP_UNPROCESSENTITY  = 422,		/* RFC-4918 */
  HTTP_LOCKED           = 423,		/* RFC-4918 */
  HTTP_FAILEDDEP        = 424,		/* RFC-4918 */
  HTTP_UPGRADE          = 426,
  HTTP_PRECONDITION2    = 428,		/* RFC-6585 */
  HTTP_TOOMANYREQUESTS  = 429,		/* RFC-6585 */
  HTTP_REQHDR2BIG       = 431,		/* RFC-6585 */
  HTTP_LOGINTIMEOUT     = 440,		/* IIS */
  HTTP_NORESPONSE       = 444,		/* nginx */
  HTTP_RETRYWITH        = 449,		/* IIS */
  HTTP_MSBLOCK          = 450,		/* U */
  HTTP_LEGALCENSOR      = 451,
  HTTP_SSLCERTERR       = 495,		/* nginx */
  HTTP_SSLCERTREQ       = 496,		/* nginx */
  HTTP_HTTP2HTTPS       = 497,		/* nginx */
  HTTP_INVALIDTOKEN     = 498,		/* U */
  HTTP_TOKENREQ         = 499,		/* U */

	/*-----------------------------------------
	; Server Errors
	;-----------------------------------------*/

  HTTP_ISERVERERR	= 500,		/* 1.0	*/
  HTTP_NOTIMP		= 501,		/* 1.0	*/
  HTTP_BADGATEWAY	= 502,		/* 1.0	*/
  HTTP_NOSERVICE	= 503,		/* 1.0	*/
  HTTP_GATEWAYTIMEOUT	= 504,		/* 1.1	*/
  HTTP_HTTPVERSION 	= 505,		/* 1.1	*/
  HTTP_VARIANTALSO      = 506,		/* RFC-2295 */
  HTTP_ENOSPC           = 507,		/* RFC-4918 */
  HTTP_LOOP             = 508,		/* RFC-5842 */
  HTTP_EXCEEDBW         = 509,		/* U */
  HTTP_NOTEXTENDED      = 510,		/* RFC-2774 */
  HTTP_NETAUTHREQ       = 511,		/* RFC-6585 */
  HTTP_UNKNOWN          = 520,		/* CloudFlare */
  HTTP_BLACKHAWKDOWN    = 521,		/* CloudFlare */
  HTTP_CONNECTIONTIMEOUT = 522,		/* CloudFlare */
  HTTP_ORIGINUNREACHABLE = 523,		/* CloudFlare */
  HTTP_TIMEOUT500       = 524,		/* CloudFlare */
  HTTP_SSLHANDSHAKE     = 525,		/* CloudFlare */
  HTTP_SSLINVALID       = 526,		/* CloudFlare */
  HTTP_FROZEN           = 530,		/* U */

} http__e;

extern char	*get_remote_user		(void);
extern bool	 authenticate_author		(Request);
extern void	 notify_emaillist		(Request);
extern int	 entry_add			(Request);
extern void	 fix_entry			(Request);
extern void	 generic_cb			(const char *const,FILE *const,void *);
extern void	 dbcritical			(const char *const);
extern char	*entity_conversion		(const char *);
extern char	*entity_encode			(const char *);
extern FILE	*fentity_encode_onread		(FILE *const);
extern FILE	*fentity_encode_onwrite		(FILE *const);

#endif

