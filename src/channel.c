#include "alloc.h"
#include "channel.h"
#include "event.h"
#include "buffer.h"
#include "evloop.h"
#include "evop.h"
#include "hash.h"
#include "logger.h"

rb_channel_t *
rb_chl_init(rb_evloop_t *loop)
{
    rb_channel_t *chl = rb_malloc(sizeof(rb_channel_t));

    chl->loop = loop;
    rb_ev_set(&chl->ev, -1, 0, 0);
    chl->input = rb_buffer_init();
    chl->output = rb_buffer_init();
    rb_chl_set_cb(chl, NULL, NULL, NULL, NULL, NULL, NULL);

    return chl;
}

void
rb_chl_add(rb_channel_t *chl)
{
    rb_hash_insert(chl->loop->chlist, chl->ev.ident, chl);
    rb_log_debug("fd=%d added chlist<fd,chl>", chl->ev.ident);
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

static void
rb_chl_enable_event(rb_channel_t *chl , int events)
{
    rb_event_t ev;
    chl->ev.events |= events;
    rb_ev_set(&ev, chl->ev.ident, events, 0);
    chl->loop->evsel->add(chl->loop->evop, &ev);
    rb_log_debug("fd=%d enable %s", chl->ev.ident, event_str[events]);
}

static void
rb_chl_disable_event(rb_channel_t *chl, int events)
{
    rb_event_t ev;
    chl->ev.events &= ~events;
    rb_ev_set(&ev, chl->ev.ident, events, 0);
    chl->loop->evsel->del(chl->loop->evop, &ev);
    rb_log_debug("fd=%d disable %s", chl->ev.ident, event_str[events]);
}

void
rb_chl_enable_read(rb_channel_t *chl)
{
    rb_chl_enable_event(chl, RB_EV_READ);
}

void
rb_chl_enable_write(rb_channel_t *chl)
{
    rb_chl_enable_event(chl, RB_EV_WRITE);
}

void
rb_chl_disable_read(rb_channel_t *chl)
{
    rb_chl_disable_event(chl, RB_EV_READ);
}

void
rb_chl_disable_write(rb_channel_t *chl)
{
    rb_chl_disable_event(chl, RB_EV_WRITE);
}
