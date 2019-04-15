#ifndef _RIBEV_EVOP_H
#define _RIBEV_EVOP_H

#include <sys/types.h>
#include "fwd.h"

typedef struct rb_evop {
    void *(*init)(void);    
    void (*add)(void *, rb_event_t *);
    void (*del)(void *, rb_event_t *);
    int (*dispatch)(rb_evloop_t *, void *, int64_t);
    void (*dealloc)(void *);
} rb_evop_t;

extern const rb_evop_t *evops[];
extern const int8_t evopsnum;

#endif /* _RIBEV_EVOP_H */
