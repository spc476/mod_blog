
#include <stdio.h>
#include <stdlib.h>

#include "wbtum.h"

int main(int argc,char *argv[])
{
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s tumbler ... \n",argv[0]);
    return EXIT_FAILURE;
  }
  
  for (int i = 1 ; i < argc ; i++)
  {
    tumbler__s tum;
    bool       rc;
    
    rc = tumbler_new(&tum,argv[i]);
    
    printf(
            "tumbler:  %s\n"
            "rc:       %s\n"
            "start:    %d/%02d/%02d.%d\t%s\n"
            "stop:     %d/%02d/%02d.%d\t%s\n"
            "file:     %s\n"
            "redirect: %s\n"
            "range:    %s\n"
            "filename: %s\n"
            "\n",
            argv[1],
            rc           ? "true" : "false",
            tum.start.year,tum.start.month,tum.start.day,tum.start.part,
            tum.stop.year, tum.stop.month, tum.stop.day, tum.stop.part,
            tum.file     ? "true" : "false",
            tum.redirect ? "true" : "false",
            tum.range    ? "true" : "false",
            tum.filename
      );
  };
  
  return EXIT_SUCCESS;
}
