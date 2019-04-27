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
#include "log.h"

#define RB_EP_INIT_NEVENTS 64

typedef struct rb_epoll {
    int epfd;
    int nevents;
    int added_events;
    rb_vector_t *evlist;
} rb_epoll_t;

#define rb_epoll_entry(ep, i) \
    ((struct epoll_event *)rb_vector_entry((ep)->evlist, i))
/*
 * 将自定义事件转换为传递给内核的EPOLL事件
 */
#define rb_epoll_set(ev, epev) \
    do { \
        (epev).events = 0; \
        if ((ev) & RB_EV_READ) \
            (epev).events |= EPOLLIN; \
        if ((ev) & RB_EV_WRITE) \
            (epev).events |= EPOLLOUT; \
    } while (0)

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
    struct epoll_event epev;
    epev.data.fd = ev->ident;
    rb_epoll_set(ev->events, epev);
    if (epoll_ctl(ep->epfd, EPOLL_CTL_ADD, ev->ident, &epev) < 0) {
        if (errno == EEXIST) {
            /* 修改事件 */
            rb_epoll_set(ev->revents, epev);
            if (epoll_ctl(ep->epfd, EPOLL_CTL_MOD, ev->ident, &epev) < 0)
                rb_log_warn("");
        } else
            rb_log_warn("");
    } else {
        /* 注册fd */
        if (++ep->added_events >= ep->nevents) {
            ep->nevents *= 2;
            rb_vector_reserve(ep->evlist, ep->nevents);
        }
        rb_log_debug("");
    }
}

static void
rb_epoll_del(void *arg, rb_event_t *ev)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    struct epoll_event epev;
    epev.data.fd = ev->ident;
    rb_epoll_set(ev->revents, epev);
    if (epoll_ctl(ep->epfd, EPOLL_CTL_MOD, ev->ident, &epev) < 0)
        rb_log_warn("");
}

static void
rb_epoll_remove(void *arg, int fd)
{
    rb_epoll_t *ep = (rb_epoll_t *)arg;
    /* EPOLL_CTL_DEL会从内核关注的fd列表中移除fd，events参数将被忽略，
     * 所以可以设置为NULL
     */
    if (epoll_ctl(ep->epfd, EPOLL_CTL_DEL, fd, NULL) < 0)
        rb_log_warn("epoll_ctl");
}

static void
rb_epoll_set_revents(rb_channel_t *chl, int events)
{
    chl->ev.revents = 0;
    if (events & EPOLLIN)
        chl->ev.revents |= RB_EV_READ;
    if (events & EPOLLOUT)
        chl->ev.revents |= RB_EV_WRITE;
    if (events & (EPOLLERR | EPOLLNVAL))
        chl->ev.revents |= RB_EV_ERROR;
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
            rb_epoll_set_revents(chl, evlist[i].events);
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
    rb_epoll_remove,
    rb_epoll_dispatch,
    rb_epoll_dealloc,
};

#endif /* RB_HAVE_EPOLL */
