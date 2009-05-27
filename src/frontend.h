
#ifndef FRONTEND_H
#define FRONTEND_H

typedef struct rflags
{
  unsigned int stdin      : 1;
  unsigned int emailin    : 1;
  unsigned int filein     : 1;
  unsigned int cgiin      : 1;
  unsigned int update     : 1;
  unsigned int preview    : 1;
  unsigned int regenerate : 1;
} RFlags;

typedef struct request
{
  int     (*command)(struct request *);
  RFlags    f;
  Cgi       cgi;
  Buffer    bin;
  Buffer    lbin;
  Buffer    bout;
  Buffer    lbout;
  FILE     *fpin;
  FILE     *fpout;
  char     *update;
  char     *author;
  char     *name;
  char     *title;
  char     *class;
  char     *date;
  char     *body;
  char     *reqtumbler;
  Tumbler  tumbler;
} *Request;  

typedef struct dflags
{
  unsigned int fullurl    : 1;
  unsigned int reverse    : 1;
  unsigned int cgi        : 1;
  unsigned int navigation : 1;
  unsigned int navprev    : 1;
  unsigned int navnext    : 1;
} DFlags;

typedef struct display
{
  struct chunk_callback *callbacks;
  size_t                 numcb;
  struct tm              begin;
  struct tm              now;
  struct tm              updatetime;
  struct tm              previous;
  struct tm              next;
  FILE                  *htmldump;
  DFlags                 f;
  int                    navunit;
} *Display;

#endif
