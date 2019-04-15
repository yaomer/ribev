#include "config.h"

#ifdef RB_HAVE_EPOLL

#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include "evloop.h"
#include "event.h"
#include "evop.h"
#include "vector.h"
#include "alloc.h"
#include "event.h"
#include "logger.h"

#define RB_EP_INIT_NEVENTS 64

typedef struct rb_epoll {
    int epfd;
    int nevents;
    int added_events;
    rb_vector_t *evlist;
} rb_epoll_t;

#define rb_epoll_entry(ep, i) \
    ((struct epoll_event *)rb_vector_entry((ep)->evlist, i))

static void *
rb_epoll_init(void)
{
    rb_epoll_t *ep = rb_malloc(sizeof(rb_epoll_t));

    ep->epfd = epoll_create(1);
    ep->added_events = 0;
    ep->nevents = RB_EP_INIT_NEVENTS;
    ep->evlist = rb_vector_init(sizeof(struct epoll_event));
    rb_vector_reserve(ep->evlist, ep->nevents);

    return (void *)ep;
}

static void
rb_epoll_add(void *arg, rb_event_t *ev)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    struct epoll_event evt;
    if (ev->events & RB_EV_READ)
        evt.events = EPOLLIN;
    else if (ev->events & RB_EV_WRITE)
        evt.events = EPOLLOUT;
    if (epoll_ctl(ep->epfd, EPOLL_CTL_ADD, ev->ident, &evt) < 0) {
        /* 重复添加fd时，epoll_ctl()返回EEXIST，我们可以
         * 忽略这个错误来达到更改事件的目的。
         */
        if (errno != EEXIST)
            rb_log_error("epoll_ctl");
    } else {
        /* 添加一个新的fd */
        if (++ep->added_events >= ep->nevents) {
            ep->nevents *= 2;
            rb_vector_reserve(ep->evlist, ep->nevents);
        }
    }
}

static void
rb_epoll_del(void *arg, rb_event_t *ev)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    if (epoll_ctl(ep->epfd, EPOLL_CTL_DEL, ev->ident, NULL) < 0)
        rb_log_error("epoll_ctl");
}

static int
rb_epoll_dispatch(rb_evloop_t *loop, void *arg, int64_t timeout)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    struct epoll_event *evlist = rb_epoll_entry(ep, 0);
    int nready = epoll_wait(ep->epfd, evlist, ep->nevents, timeout);

    if (nready > 0) {
        for (int i = 0; i < nready; i++) {
            rb_channel_t *chl = rb_search_chl(loop, evlist[i].ident);
            chl->ev.revents = evlist[i].events;
            rb_queue_push(loop->active_chls, chl);
        }
    }
    return nready;
}

static void
rb_epoll_dealloc(void *arg)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    rb_vector_destroy(&ep->evlist);
    close(ep->epfd);
    rb_free(ep);
}

const rb_evop_t epops = {
    rb_epoll_init,
    rb_epoll_add,
    rb_epoll_del,
    rb_epoll_dispatch,
    rb_epoll_dealloc,
};

#endif /* RB_HAVE_EPOLL */
