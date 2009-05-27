/*********************************************
*
* Copyright 2002 by Sean Conner.  All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cgilib/ddt.h>
#include <cgilib/mail.h>

#if 0
#define SENDMAIL	"/usr/sbin/sendmail"
#endif

/**************************************************************************/

int send_message(
                  const char *from,
		  const char *replyto,
		  const char *to,
		  const char *subject,
		  const char *message
		)
{
  Email email;

  email          = EmailNew();
  email->from    = from;
  email->to      = to;
  email->subject = subject;

  if (replyto) email->replyto = replyto;

  LineS(email->body,message);
  EmailSend(email);
  EmailFree(email);
  return(0);

#if 0

  FILE      *fpin;
  FILE      *fpout;
  time_t     now;
  struct tm *ptm;
  char       cmd   [BUFSIZ];
  char       date  [BUFSIZ];
  char       buffer[BUFSIZ];

  ddt(from    != NULL);
  ddt(to      != NULL);
  ddt(subject != NULL);
  ddt(message != NULL);

  fpin = fopen(message,"r");  
  if (fpin == NULL)
    return(1);
    
  sprintf(cmd,SENDMAIL " %s",to);
  fpout = popen(cmd,"w");
  if (fpout == NULL)
  {
    fclose(fpin);
    return(1);
  }
  
  now = time(NULL);
  ptm = localtime(&now);
  
  strftime(date,BUFSIZ,"%a, %d %b %Y %H:%M:%S %Z",ptm);
  
  fprintf(fpout,"From: %s\n",from);
  if (replyto)
    fprintf(fpout,"Reply-To: %s\n",replyto);
  
  fprintf(
           fpout,
	   "To: %s\n"
           "Subject: %s\n"
           "Date: %s\n"
           "\n",
	   to,
           subject,
           date
         );
         
  while(fgets(buffer,BUFSIZ,fpin))
    fputs(buffer,fpout);
  
  fputs("\n.\n",fpout);
  
  fclose(fpout);
  fclose(fpin);
  
  return(0);
#endif
}

/************************************************************************/
