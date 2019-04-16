#include "config.h"

#ifdef RB_HAVE_KQUEUE

#include <stdio.h>
#include <unistd.h>
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
#include "log.h"

#define RB_KQ_INIT_NEVENTS 64

typedef struct rb_kqueue {
    int kqfd;
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

    k->kqfd = kqueue();
    k->added_events = 0;
    k->nevents = RB_KQ_INIT_NEVENTS;
    k->evlist = rb_vector_init(sizeof(struct kevent));
    rb_vector_reserve(k->evlist, k->nevents);

    return (void *)k;
}

/*
 * EVFILT_XXX被定义的常量值如下：
 * #define EVFILT_READ  (-1)
 * #define EVFILT_WRITE (-2)
 * 所以它们之间不能进行按位与或运算。
 * 正因为如此kqueue每次只返回单个事件，如可读或可写，而不会
 * 返回同时可读可写。
 */

/*
 * kqueue的EV_ADD可以重复添加事件，即除了添加事件之外，也可以
 * 用来修改事件；而epoll只能用EPOLL_CTL_MOD来修改事件。
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
    if (kevent(k->kqfd, &kev, 1, NULL, 0, NULL) < 0)
        rb_log_warn("kevent");
}

/*
 * kqueue的EV_DELETE只会删除指定fd上的事件，而epoll的EPOLL_CTL_DEL
 * 则会删除指定的fd。
 */
static void
rb_kqueue_del(void *arg, rb_event_t *ev)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    struct kevent kev;
    if (ev->events & RB_EV_READ)
        EV_SET(&kev, ev->ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    else if (ev->events & RB_EV_WRITE)
        EV_SET(&kev, ev->ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    if (kevent(k->kqfd, &kev, 1, NULL, 0, NULL) < 0)
        rb_log_warn("kevent");
}

/*
 * 不清楚如何移除一个fd，emmmm
 */
static void
rb_kqueue_remove(void *arg, int fd)
{
    close(fd);
}

static void
rb_kqueue_set_revents(rb_channel_t *chl, int filter)
{
    chl->ev.revents = 0;
    if (filter == EVFILT_READ)
        chl->ev.revents |= RB_EV_READ;
    else if (filter == EVFILT_WRITE)
        chl->ev.revents |= RB_EV_WRITE;
    else if (filter == EVFILT_EXCEPT)
        chl->ev.revents |= RB_EV_ERROR;
}

static int
rb_kqueue_dispatch(rb_evloop_t *loop, void *arg, int64_t timeout)
{
    rb_kqueue_t *k = (rb_kqueue_t *)arg;
    struct kevent *evlist = rb_kqueue_entry(k, 0);
    struct timespec tsp;
    int nready;

    if (timeout > 0) {
        tsp.tv_sec = timeout / 1000;
        tsp.tv_nsec = (timeout % 1000) * 1000 * 1000;
        nready = kevent(k->kqfd, NULL, 0, evlist, k->nevents, &tsp);
    } else
        nready = kevent(k->kqfd, NULL, 0, evlist, k->nevents, NULL);

    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            rb_channel_t *chl = rb_search_chl(loop, evlist[i].ident);
            rb_kqueue_set_revents(chl, evlist[i].filter);
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
    close(k->kqfd);
    rb_free(k);
}

const rb_evop_t kqops = {
    rb_kqueue_init,
    rb_kqueue_add,
    rb_kqueue_del,
    rb_kqueue_remove,
    rb_kqueue_dispatch,
    rb_kqueue_dealloc,
};

#endif /* RB_HAVE_KQUEUE */
