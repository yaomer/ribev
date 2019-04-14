#include "evloop.h"
#include "event.h"
#include "evop.h"

static void *
rb_epoll_init(void)
{
    return 0;
}

static void
rb_epoll_add(void *arg, rb_event_t *ev)
{

}

static void
rb_epoll_del(void *arg, rb_event_t *ev)
{

}

static int
rb_epoll_dispatch(rb_evloop_t *loop, void *arg, int64_t timeout)
{
    return 0;
}

static void
rb_epoll_dealloc(void *arg)
{

}

const rb_evop_t epops = {
    rb_epoll_init,
    rb_epoll_add,
    rb_epoll_del,
    rb_epoll_dispatch,
    rb_epoll_dealloc,
};
