#ifndef _RIBEV_EVTHR_H
#define _RIBEV_EVTHR_H

#include <pthread.h>
#include <stdatomic.h>
#include "fwd.h"

typedef struct rb_evthr {
    rb_evloop_t *loop;
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    atomic_int start;
} rb_evthr_t;

rb_evthr_t *rb_evthr_init(void);
void rb_evthr_run(rb_evthr_t *evthr);
void rb_evthr_quit(rb_evthr_t *evthr);

#endif /* _RIBEV_EVTHR_H */
