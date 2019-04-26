#ifndef _RIBEV_THREADPOOL_H
#define _RIBEV_THREADPOOL_H

#include <pthread.h>
#include <stdatomic.h>
#include "fwd.h"

typedef struct rb_threadpool {
    rb_queue_t *qtask;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t *tid;
    int threads;
    atomic_int quit;
} rb_threadpool_t;

rb_threadpool_t *rb_threadpool_init(int threads);
void rb_threadpool_add(rb_threadpool_t *pool, rb_task_t *task);
void rb_threadpool_destroy(rb_threadpool_t **_pool);

#endif /* _RIBEV_THREADPOOL_H */
