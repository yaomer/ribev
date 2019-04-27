#include "evthr.h"
#include "evloop.h"
#include "lock.h"
#include "alloc.h"

/*
 * 用线程封装loop，即[one loop per thread]
 */

static void
rb_evthr_destroy(rb_evthr_t *evthr)
{
    rb_free(evthr->loop);
    rb_lock_destroy(&evthr->mutex);
    rb_cond_destroy(&evthr->cond);
    rb_free(evthr);
}

static void *
rb_evthr_func(void *arg)
{
    rb_evthr_t *evthr = (rb_evthr_t *)arg;
    evthr->loop = rb_evloop_init();
    while (!evthr->start)
        rb_wait(&evthr->cond, &evthr->mutex);
    rb_evloop_run(evthr->loop);
    rb_evthr_destroy(evthr);
    return 0;
}

rb_evthr_t *
rb_evthr_init(void)
{
    rb_evthr_t *evthr = rb_malloc(sizeof(rb_evthr_t));

    rb_lock_init(&evthr->mutex);
    rb_cond_init(&evthr->cond);
    evthr->start = 0;
    pthread_create(&evthr->tid, NULL, rb_evthr_func, evthr);

    return evthr;
}

void
rb_evthr_run(rb_evthr_t *evthr)
{
    evthr->start = 1;
    rb_notify(&evthr->cond);
}

void
rb_evthr_quit(rb_evthr_t *evthr)
{
    rb_evloop_quit(evthr->loop);
}
