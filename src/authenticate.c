
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cgi/memory.h>
#include <cgi/ddt.h>
#include <cgi/util.h>

#ifdef USE_GDBM
#  include <gdbm.h>
#endif

#ifdef USE_DB
#  include <limits.h>
#  include <sys/types.h>
#  include <db.h>
#endif

/*************************************************************************/

#ifdef USE_GDBM

  int authenticate_author(void)
  {
    GDBM_FILE list;
    datum     key;
    int       rc;

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);

    list = gdbm_open(g_authorfile,DB_BLOCK,GDBM_READER,0,dbcritical);
    if (list == NULL)
      return(FALSE);

    key.dptr  = m_author;
    key.dsize = strlen(m_author) + 1;
    rc        = gdbm_exists(list,key);
    gdbm_close(list);
    return(rc);
  }

/***********************************************************************/

#elif defined (USE_DB)

  int authenticate_author(void)
  {
    DB  *list;
    DBT  key;
    DBT  data;
    int  rc;

    /*--------------------------------------------------------------
    ; this version will replace the login name with the full name,
    ; assuming it's defined in the database file as the (hardcoded)
    ; third field of the value portion returned.
    ;---------------------------------------------------------------*/

    ddt(author != NULL);

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);

    list = dbopen(g_authorfile,O_RDONLY,0644,DB_HASH,NULL);
    if (list == NULL)
      return(FALSE);

    key.data = m_author;
    key.size = strlen(m_author);
    rc       = (list->get)(list,&key,&data,0);
    (list->close)(list);
    if (rc) return(FALSE);
    
    {
      char   *tmp;
      char   *p;
      char   *q;
      size_t  i;

      tmp = MemAlloc(data.size + 1);
      memset(tmp,0,data.size + 1);
      memcpy(tmp,data.data,data.size);
 
      /*----------------------------------------------------------
      ; For now, we assume that if using Berkeley DB, we are doing
      ; so because we want to use the DB file used by Apache, and
      ; that's set up as key => passwd ':' group ':' other
      ; so we look for two colons, and take what is past there as
      ; the real name.  
      ;-----------------------------------------------------------*/
 
      for (i = 0 , p = tmp ; i < 2 ; i++)
      {
        p = strchr(p,':');
        if (p == NULL)
        {
          MemFree(tmp,key.size + 1);
          return(TRUE);
        }
        p++;
      }
 
      /*----------------------------------------------------------
      ; there may be more fields, so check for the end of this field,
      ; and if so marked, replace it with an end of string marker.
      ;-----------------------------------------------------------*/
 
      q = strchr(p,':');
      if (q) *q = '\0';
 
      /*------------------------------------------------------
      ; There is a potential memory leak here, as m_author may
      ; have been allocated earlier.  But since it's constantly
      ; defined to begin with, and there is no real portable way
      ; to determine if the pointer has been allocated at run time
      ; we can't really test it, so we loose some memory here.
      ;
      ; This is expected, but since this program doesn't run
      ; infinitely, this is somewhat okay.
      ;------------------------------------------------------*/
 
      m_author = dup_string(p);
      MemFree(tmp,key.size + 1);
    }

    return(TRUE);
  }

/*************************************************************************/

#elif defined (USE_HTPASSWD)

  static size_t breakline(char **dest,size_t dsize,FILE *fpin)
  {
    static char buffer[BUFSIZ];	/* non-thread-safe! */
    char        *p;
    char        *nl;
    size_t       cnt;
    
    p = fgets(buffer,sizeof(buffer),fpin);
    if (p == NULL) return(0);

    nl = strchr(buffer,'\n');	/* remove trailing newline */
    if (nl) *nl = '\0';

    for (cnt = 0 ; cnt < dsize ; cnt++)
    {
      dest[cnt] = p;
      p = strchr(p,':');
      if (p == NULL) return(cnt + 1);
      *p++ = '\0';
    }
    return(cnt);
  }

  /*------------------------------------------------------*/

  int authenticate_author(void)
  {
    FILE   *fpin;
    char   *lines[10];	/* 10 fields max */
    size_t  cnt;

    if (g_authorfile == NULL)
      return (strcmp(m_author,g_author) == 0);
 
    fpin = fopen(g_authorfile,"r");
    if (fpin == NULL)
      return(FALSE);
 
    while((cnt = breakline(lines,10,fpin)))
    {
      if (strcmp(m_author,lines[0]) == 0)
      {
        /*--------------------------------------------------
        ; A potential memory leak---see the comment above in 
        ; the USE_DB version of this routine
        ;---------------------------------------------------*/
        
        if (cnt >= 3)
          m_author = dup_string(lines[2]);

        fclose(fpin);
        return(TRUE);
      }
    }
    fclose(fpin);
    return(FALSE);
  }

#endif

/**************************************************************************/

