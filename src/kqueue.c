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

typedef struct rb_kqueue {
    int kq;
    rb_vector_t *evlist;
} rb_kqueue_t;

#define rb_kq_entry(k, i) \
    ((struct kevent *)rb_vector_entry((k)->evlist, i))

static rb_kqueue_t *
rb_kqueue_init(void)
{
    rb_kqueue_t *k = rb_malloc(sizeof(rb_kqueue_t));

    k->kq = kqueue();
    k->evlist = rb_vector_init(sizeof(struct kevent));

    return k;
}

static void
rb_kqueue_add(rb_kqueue_t *k, rb_event_t *ev)
{
    struct kevent kev;
    EV_SET(&kev, ev->ident, ev->events, EV_ADD, NULL, 0, NULL);
    if (kevent(k->kq, &kev, 1, NULL, 0, NULL) < 0)
        ;
}

static void
rb_kqueue_del(rb_kqueue_t *k, rb_event_t *ev)
{
    struct kevent kev;
    EV_SET(&kev, ev->ident, ev->events, EV_DELETE, NULL, 0, NULL);
    if (kevent(k->kq, &kev, 1, NULL, 0, NULL) < 0)
        ;
}

static int
rb_kqueue_dispatch(rb_evloop_t *loop, rb_kqueue_t *k, int64_t timeout)
{
    struct timespec tsp;
    struct kevent *evlist = rb_kq_entry(k, 0);
    int nevents = rb_vector_max_size(k->evlist);
    tsp.tv_sec = timeout / 1000;
    tsp.tv_nsec = (timeout % 1000) * 1000 * 1000;
    int nready = kevent(k->kq, NULL, 0, evlist, nevents, &tsp);

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
rb_kqueue_dealloc(rb_kqueue_t *k)
{
    rb_vector_destroy(&k->evlist);
    rb_free(k);
}

const rb_evop_t kqops = {
    "kqueue",
    rb_kqueue_init,
    rb_kqueue_add,
    rb_kqueue_del,
    rb_kqueue_dispatch,
    rb_kqueue_dealloc,
};
