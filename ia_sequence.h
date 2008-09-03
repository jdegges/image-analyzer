#ifndef _H_IA_SEQUENCE
#define _H_IA_SEQUENCE

#include <FreeImage.h>
#include "common.h"
#include "image_analyzer.h"

/* opens text file containing list of images / device */
ia_seq_t*           ia_seq_open ( ia_param_t* p );

/* closes ia stream */
inline void         ia_seq_close ( ia_seq_t* s );

/* return pointer to an image at specified relative position in stream */
int                 ia_seq_getimage ( ia_seq_t* s );

int                 ia_seq_probelist( ia_seq_t* s );

/* save ia_seq_t->iar */
void*               ia_seq_saveimage ( void* ptr );

#endif
