#ifndef _RIBEV_EVLOOP_H
#define _RIBEV_EVLOOP_H

#include <pthread.h>
#include <stdatomic.h>
#include "fwd.h"
#include "hash.h"

typedef struct rb_evloop {
    void *evop;
    const rb_evop_t *evsel;
    rb_hash_t *chlist;    
    rb_timer_t *timer;
    rb_queue_t *active_chls;
    rb_queue_t *ready_chls;
    rb_queue_t *qtask;
    pthread_mutex_t mutex;
    int wakefd[2];
    atomic_int quit;
    int tid;
} rb_evloop_t;

#define rb_search_chl(loop, fd) \
    ((rb_channel_t *)rb_hash_search((loop)->chlist, fd))

rb_evloop_t *rb_evloop_init(void);
void rb_evloop_run(rb_evloop_t *loop);
void rb_evloop_quit(rb_evloop_t *loop);

int rb_in_loop_thread(rb_evloop_t *loop);
void rb_wakeup(rb_evloop_t *loop);
void rb_wakeup_read(rb_channel_t *chl);
void rb_run_in_loop(rb_evloop_t *loop, rb_task_t *task);

#endif /* _RIBEV_EVLOOP_H */
