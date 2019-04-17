#include "evthr.h"
#include "evloop.h"
#include "lock.h"
#include "alloc.h"

/*
 * 用线程封装loop，即one loop per thread
 */

static void *
rb_evthr_func(void *arg)
{
    rb_evthr_t *evthr = (rb_evthr_t *)arg;
    evthr->loop = rb_evloop_init();
    while (!evthr->start)
        rb_wait(&evthr->cond, &evthr->mutex);
    rb_evloop_run(evthr->loop);
    return 0;
}

rb_evthr_t *
rb_evthr_init(void)
{
    rb_evthr_t *evthr = rb_malloc(sizeof(rb_evthr_t));

    rb_lock_init(&evthr->mutex);
    rb_cond_init(&evthr->cond);
    evthr->start = 0;
    rb_creat_thread(&evthr->tid, rb_evthr_func, evthr);

    return evthr;
}

void
rb_evthr_run(rb_evthr_t *evthr)
{
    evthr->start = 1;
    rb_notify(&evthr->cond);
}
