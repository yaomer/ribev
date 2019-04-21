#include <stdio.h>
#include <unistd.h>
#include "alloc.h"
#include "channel.h"
#include "event.h"
#include "buffer.h"
#include "evloop.h"
#include "evop.h"
#include "hash.h"
#include "vector.h"
#include "task.h"
#include "log.h"

rb_channel_t *
rb_chl_init(rb_evloop_t *loop)
{
    rb_channel_t *chl = rb_malloc(sizeof(rb_channel_t));

    chl->loop = loop;
    rb_ev_set(&chl->ev, -1, 0, 0);
    chl->input = rb_buffer_init();
    chl->output = rb_buffer_init();
    rb_chl_set_cb(chl, NULL, NULL, NULL, NULL);

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

/*
 * 开启events指定的事件
 */
static void
rb_chl_enable_event(rb_channel_t *chl , int events)
{
    rb_event_t ev;
    chl->ev.events |= events;
    /* ev.events被设置为当前待操作的事件,
     * ev.revents被设置为当前已关注的所有事件。
     */
    rb_ev_set(&ev, chl->ev.ident, events, chl->ev.events);
    chl->loop->evsel->add(chl->loop->evop, &ev);
    rb_log_debug("fd=%d enable %s", chl->ev.ident, rb_eventstr(events));
}

/*
 * 关闭events指定的事件
 */
static void
rb_chl_disable_event(rb_channel_t *chl, int events)
{
    rb_event_t ev;
    chl->ev.events &= ~events;
    rb_ev_set(&ev, chl->ev.ident, events, chl->ev.events);
    chl->loop->evsel->del(chl->loop->evop, &ev);
    rb_log_debug("fd=%d disable %s", chl->ev.ident, rb_eventstr(events));
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

/*
 * 向loop中注册一个新的fd，默认开启RB_EV_READ事件
 */
static void
__rb_chl_add(void **argv)
{
    rb_channel_t *chl = (rb_channel_t *)argv[0];
    rb_hash_insert(chl->loop->chlist, chl->ev.ident, chl);
    rb_chl_enable_read(chl);
}

void
rb_chl_add(rb_channel_t *chl)
{
    rb_task_t *t = rb_alloc_task(1);
    t->callback = __rb_chl_add;
    t->argv[0] = chl;
    rb_run_in_loop(chl->loop, t);
}

/*
 * 从loop中移除一个fd
 */
static void
__rb_chl_del(void **argv)
{
    rb_channel_t *chl = (rb_channel_t *)argv[0];
    /* 这两行顺序不能变 */
    chl->loop->evsel->remove(chl->loop->evop, chl->ev.ident);
    rb_hash_delete(chl->loop->chlist, chl->ev.ident);
}

void
rb_chl_del(rb_channel_t *chl)
{
    rb_task_t *t = rb_alloc_task(1);
    t->callback = __rb_chl_del;
    t->argv[0] = chl;
    rb_run_in_loop(chl->loop, t);
}

void
rb_free_chl(void *arg)
{
    rb_channel_t *chl = (rb_channel_t *)arg;
    close(chl->ev.ident);
    rb_buffer_destroy(&chl->input);
    rb_buffer_destroy(&chl->output);
    rb_free(chl);
}
