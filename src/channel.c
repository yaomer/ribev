#include "alloc.h"
#include "channel.h"
#include "event.h"
#include "buffer.h"
#include "evloop.h"
#include "evop.h"

rb_channel_t *
rb_chl_init(rb_evloop_t *loop)
{
    rb_channel_t *chl = rb_malloc(sizeof(rb_channel_t));

    chl->loop = loop;
    chl->ev.ident = -1;
    chl->ev.events = 0;
    chl->ev.revents = 0;
    chl->input = rb_buffer_init();
    chl->output = rb_buffer_init();
    chl->eventcb = NULL;
    chl->readcb = NULL;
    chl->writecb = NULL;
    chl->msgcb = NULL;
    chl->packcb = NULL;
    chl->unpackcb = NULL;

    return chl;
}

int
rb_chl_is_reading(rb_channel_t *chl)
{
    return chl->ev.events & RB_EV_READ;
}

int
rb_chl_is_writing(rb_channel_t *chl)
{
    return chl->ev.events & RB_EV_WRITE;
}

void
rb_chl_enable_read(rb_channel_t *chl)
{
    rb_event_t ev;
    chl->ev.events |= RB_EV_READ;
    rb_ev_set(ev, chl->ev.ident, RB_EV_READ, 0);
    chl->loop->evsel->add(chl->loop->evop, &ev);
}

void
rb_chl_enable_write(rb_channel_t *chl)
{
    rb_event_t ev;
    chl->ev.events |= RB_EV_WRITE;
    rb_ev_set(ev, chl->ev.ident, RB_EV_WRITE, 0);
    chl->loop->evsel->add(chl->loop->evop, &ev);
}

void
rb_chl_disable_read(rb_channel_t *chl)
{
    rb_event_t ev;
    chl->ev.events &= ~RB_EV_READ;
    rb_ev_set(ev, chl->ev.ident, RB_EV_READ, 0);
    chl->loop->evsel->del(chl->loop->evop, &ev);
}

void
rb_chl_disable_write(rb_channel_t *chl)
{
    rb_event_t ev;
    chl->ev.events &= ~RB_EV_WRITE;
    rb_ev_set(ev, chl->ev.ident, RB_EV_WRITE, 0);
    chl->loop->evsel->del(chl->loop->evop, &ev);
}
