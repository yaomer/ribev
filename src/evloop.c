#include <errno.h>
#include "alloc.h"
#include "evop.h"
#include "evloop.h"
#include "queue.h"
#include "channel.h"

__thread rb_evloop_t *__onlyloop = NULL;

rb_evloop_t *
rb_evloop_init(void)
{
    rb_evloop_t *loop = rb_malloc(sizeof(rb_evloop_t));
    if (__onlyloop)
        ;
    else
        __onlyloop = loop;

    loop->evsel = evops[RB_KQUEUE];
    loop->evop = loop->evsel->init();
    loop->chlist = rb_hash_init();
    loop->active_chls = rb_queue_init();
    loop->quit = 0;

    return loop;
}

static void
rb_run_io(rb_evloop_t *loop)
{
    while (!rb_queue_is_empty(loop->active_chls)) {
        rb_channel_t *chl = rb_queue_front(loop->active_chls);
        if (chl->eventcb)
            chl->eventcb(chl);
        rb_queue_pop(loop->active_chls);
    }
}

void
rb_evloop_run(rb_evloop_t *loop)
{
    while (!loop->quit) {
        int nevents = loop->evsel->dispatch(loop, loop->evop, NULL);
        if (nevents == 0) {
            ;
        } else if (nevents > 0) {
            rb_run_io(loop);
        } else {
            if (errno == EINTR
             || errno == EWOULDBLOCK
             || errno == EPROTO
             || errno == ECONNABORTED)
                ;
            else
                ;
        }
    }
}
