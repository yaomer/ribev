#ifndef _RIBEV_ALLOC_H
#define _RIBEV_ALLOC_H

#include <stddef.h>

void * rb_malloc(size_t nbytes);
void * rb_calloc(size_t n, size_t len);
void rb_free(void *p);
void rb_checkmem(void);

#endif /* _RIBEV_ALLOC_H */
