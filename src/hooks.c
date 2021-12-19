/***********************************************
*
* Copyright 2021 by Sean Conner.  All Rights Reserved.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sysexits.h>

/************************************************************************/

int run_hook(char const *tag,char const **argv)
{
  assert(tag     != NULL);
  assert(argv    != NULL);
  assert(argv[0] != NULL);
  
  pid_t child = fork();
  
  if (child == -1)
    return errno;
  else if (child == 0)
  {
    extern char **environ;
    int devnull = open("/dev/null",O_RDWR);
    if (devnull == -1)
      _Exit(EX_UNAVAILABLE);
    if (dup2(devnull,STDIN_FILENO) == -1)
      _Exit(EX_OSERR);
    if (dup2(devnull,STDOUT_FILENO) == -1)
      _Exit(EX_OSERR);
    if (dup2(devnull,STDERR_FILENO) == -1)
      _Exit(EX_OSERR);
    for (int fh = STDERR_FILENO + 1 ; fh <= devnull ; fh++)
      if (close(fh) == -1)
        _Exit(EX_OSERR);
    if (execve((char *)argv[0],(char **)argv,environ) == -1)
      _Exit(EX_UNAVAILABLE);
  }
  else
  {
    int status;
    
    if (waitpid(child,&status,0) != child)
      return errno;
    if (WIFEXITED(status))
    {
      if (WEXITSTATUS(status) != 0)
        syslog(LOG_ERR,"%s='%s' rc=%d",tag,argv[0],WEXITSTATUS(status));
    }
    else
      syslog(LOG_ERR,"%s='%s' terminated='%s'",tag,argv[0],strsignal(WTERMSIG(status)));
  }
  
  return 0;
}

/************************************************************************/
