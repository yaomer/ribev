#include <stdio.h>
#include <sys/event.h>
#include <sys/time.h>
#include "alloc.h"
#include "evop.h"
#include "vector.h"
#include "event.h"
#include "evloop.h"
#include "hash.h"
#include "channel.h"
#include "queue.h"

#define RB_KQ_INIT_NEVENTS 64

typedef struct rb_kqueue {
    int kq;
    int nevents;
    int added_events;
    rb_vector_t *evlist;
} rb_kqueue_t;

#define rb_kqueue_entry(k, i) \
    ((struct kevent *)rb_vector_entry((k)->evlist, i))

static void *
rb_kqueue_init(void)
{
    rb_kqueue_t *k = rb_malloc(sizeof(rb_kqueue_t));

    k->kq = kqueue();
    k->added_events = 0;
    k->nevents = RB_KQ_INIT_NEVENTS;
    k->evlist = rb_vector_init(sizeof(struct kevent));
    rb_vector_reserve(k->evlist, k->nevents);

    return (void *)k;
}

/*
 * EVFILT_XXX之间不能进行按位与或运算来判断事件
 * #define EVFILT_READ  (-1)
 * #define EVFILT_WRITE (-2)
 */

static void
rb_kqueue_add(void *arg, rb_event_t *ev)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    struct kevent kev;
    if (ev->events & RB_EV_READ) {
        EV_SET(&kev, ev->ident, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (++k->added_events >= k->nevents) {
            k->nevents *= 2;
            rb_vector_reserve(k->evlist, k->nevents);
        }
    } else if (ev->events & RB_EV_WRITE)
        EV_SET(&kev, ev->ident, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    if (kevent(k->kq, &kev, 1, NULL, 0, NULL) < 0)
        ;
}

static void
rb_kqueue_del(void *arg, rb_event_t *ev)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    struct kevent kev;
    if (ev->events & RB_EV_READ)
        EV_SET(&kev, ev->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    else if (ev->events & RB_EV_WRITE)
        EV_SET(&kev, ev->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    if (kevent(k->kq, &kev, 1, NULL, 0, NULL) < 0)
        ;
}

static int
rb_kqueue_dispatch(rb_evloop_t *loop, void *arg, int64_t timeout)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    struct timespec tsp;
    struct kevent *evlist = rb_kqueue_entry(k, 0);
    tsp.tv_sec = timeout / 1000;
    tsp.tv_nsec = (timeout % 1000) * 1000 * 1000;
    int nready = kevent(k->kq, NULL, 0, evlist, k->nevents, &tsp);

    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            rb_channel_t *chl = rb_search_chl(loop, evlist[i].ident);
            chl->ev.revents = evlist[i].filter;
            rb_queue_push(loop->active_chls, chl);
        }
    }
    return nready;
}

static void
rb_kqueue_dealloc(void *arg)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    rb_vector_destroy(&k->evlist);
    rb_free(k);
}

const rb_evop_t kqops = {
    rb_kqueue_init,
    rb_kqueue_add,
    rb_kqueue_del,
    rb_kqueue_dispatch,
    rb_kqueue_dealloc,
};