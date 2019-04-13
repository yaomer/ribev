#ifndef _RIBEV_EVLOOP_H
#define _RIBEV_EVLOOP_H

#include "fwd.h"
#include "hash.h"

typedef struct rb_evloop {
    void *evop;
    rb_evop_t *evsel;
    rb_hash_t *chlist;    
    rb_queue_t *active_chls;
    int quit;
} rb_evloop_t;

#define rb_search_chl(loop, fd) \
    ((rb_channel_t *)rb_hash_search((loop)->chlist, fd))

#endif /* _RIBEV_EVLOOP_H */
