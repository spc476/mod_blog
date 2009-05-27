
#ifndef FIX_H
#define FIX_H

#include <cgi/errors.h>
#include "chunk.h"

#define NEW	1
#define CBBODY		(ERR_APP + 4)
#define DB_BLOCK	1024

#define CALLBACKS       ((sizeof(m_callbacks) / sizeof(struct chunk_callback)) - 1)

extern struct chunk_callback m_callbacks[41];

int	authenticate_author		(Request);
int	generate_pages			(Request);
void	notify_weblogcom		(void);
void	notify_emaillist		(void);
void	fix_entry			(Request);
void	collect_body			(Buffer,Buffer);
int	tumbler_page			(FILE *,Tumbler);
void	BlogDatesInit			(void);
int	entry_add			(Request);
void	dbcritical			(char *);

#endif

