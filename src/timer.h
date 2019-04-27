#ifndef _RIBEV_TIMER_H
#define _RIBEV_TIMER_H

#include <inttypes.h>
#include "task.h"
#include "fwd.h"

typedef struct rb_timestamp {
    int64_t timeout;
    int64_t interval;
    rb_task_t *task;
} rb_timestamp_t;

typedef struct rb_timer {
    rb_vector_t *timer;
} rb_timer_t;

rb_timer_t *rb_timer_init(void);
int rb_timer_out(rb_timer_t *t);
void rb_timer_tick(rb_evloop_t *loop);
void rb_run_at(rb_evloop_t *loop, int64_t timeval, rb_task_t *t);
void rb_run_after(rb_evloop_t *loop, int64_t timeout, rb_task_t *t);
void rb_run_every(rb_evloop_t *loop, int64_t interval, rb_task_t *t);
void rb_timer_destroy(rb_timer_t **_t);

#endif /* _RIBEV_TIMER_H */
