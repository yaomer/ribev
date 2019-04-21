#include "config.h"

#ifdef RB_HAVE_POLL

#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include "evop.h"
#include "evloop.h"
#include "event.h"
#include "channel.h"
#include "vector.h"
#include "hash.h"
#include "queue.h"
#include "alloc.h"

typedef struct rb_poll {
    rb_vector_t *pollfds;
    rb_hash_t *indexs;  /* <fd, index>, index for pollfds */
} rb_poll_t;

#define rb_poll_entry(poll, i) \
    ((struct pollfd *)rb_vector_entry((poll)->pollfds, i))
#define rb_poll_index(poll, fd) \
    (*(size_t *)rb_hash_search((poll)->indexs, fd))
#define rb_poll_set(ev, pollev) \
    do { \
        (pollev).events = 0; \
        if ((ev) & RB_EV_READ) \
            (pollev).events |= POLLIN; \
        if ((ev) & RB_EV_WRITE) \
            (pollev).events |= POLLOUT; \
    } while (0)

static void *
rb_poll_init(void)
{
    rb_poll_t *poll = rb_malloc(sizeof(rb_poll_t));
    poll->pollfds = rb_vector_init(sizeof(struct pollfd));
    poll->indexs = rb_hash_init();
    rb_hash_set_free(poll->indexs, rb_free);
    return (void *)poll;
}

static void
rb_poll_add(void *arg, rb_event_t *ev)
{
    rb_poll_t *poll = (rb_poll_t *)arg;
    struct pollfd pollev;
    pollev.fd = ev->ident;
    rb_poll_set(ev->revents, pollev);
    size_t *idx = rb_hash_search(poll->indexs, pollev.fd);
    if (idx) {
        struct pollfd *pfd = rb_poll_entry(poll, *idx);
        pfd->events = pollev.events;
    } else {
        rb_vector_push(poll->pollfds, &pollev);
        idx = rb_malloc(sizeof(size_t));
        *idx = rb_vector_size(poll->pollfds) - 1;
        rb_hash_insert(poll->indexs, pollev.fd, idx);
    }
}

static void
rb_poll_del(void *arg, rb_event_t *ev)
{
    rb_poll_t *poll = (rb_poll_t *)arg;
    size_t idx = rb_poll_index(poll, ev->ident);
    struct pollfd *pfd = rb_poll_entry(poll, idx);
    pfd->events = ev->revents;
}

static void
rb_poll_remove(void *arg, int fd)
{
    rb_poll_t *poll = (rb_poll_t *)arg;
    size_t idx = rb_poll_index(poll, fd);
    size_t size = rb_vector_size(poll->pollfds) - 1;
    rb_vector_swap(poll->pollfds, idx, size);
    rb_vector_pop(poll->pollfds);
    rb_hash_delete(poll->indexs, fd);
}

static void
rb_poll_set_revents(rb_channel_t *chl, int events)
{
    chl->ev.revents = 0;
    if (events & POLLIN)
        chl->ev.revents |= RB_EV_READ;
    if (events & POLLOUT)
        chl->ev.revents |= RB_EV_WRITE;
    if (events & (POLLERR | POLLNVAL))
        chl->ev.revents |= RB_EV_ERROR;
}

static int
rb_poll_dispatch(rb_evloop_t *loop, void *arg, int64_t timeout)
{
    rb_poll_t *_poll = (rb_poll_t *)arg;
    struct pollfd *pfd = rb_poll_entry(_poll, 0);
    int fds = rb_vector_size(_poll->pollfds);
    int nready = poll(pfd, fds, timeout);
    int ret = nready;

    if (nready > 0) {
        for (int i = 0; i < fds; i++) {
            if (pfd[i].fd < 0 || pfd[i].revents == 0)
                continue;
            rb_channel_t *chl = rb_search_chl(loop, pfd[i].fd);
            rb_poll_set_revents(chl, pfd[i].revents);
            rb_queue_push(loop->active_chls, chl);
            if (--nready <= 0)
                break;
        }
    }
    return ret;
}

static void
rb_poll_dealloc(void *arg)
{
    rb_poll_t *poll = (rb_poll_t *)arg;
    rb_vector_destroy(&poll->pollfds);
    rb_hash_destroy(&poll->indexs);
    rb_free(poll);
}

const rb_evop_t pollops = {
    rb_poll_init,
    rb_poll_add,
    rb_poll_del,
    rb_poll_remove,
    rb_poll_dispatch,
    rb_poll_dealloc,
};

#endif /* RB_HAVE_POLL */
