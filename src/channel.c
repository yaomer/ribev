#include <stdio.h>
#include <string.h>
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
#include "user.h"
#include "log.h"

rb_channel_t *
rb_chl_init(rb_evloop_t *loop, rb_user_t user)
{
    rb_channel_t *chl = rb_malloc(sizeof(rb_channel_t));

    memset(chl, 0, sizeof(*chl));
    chl->loop = loop;
    chl->input = rb_buffer_init();
    chl->output = rb_buffer_init();
    rb_ev_set(&chl->ev, -1, 0, 0);
    if (user.init) {
        user.data = user.init();
        chl->user = user;
    }

    return chl;
}

void
rb_chl_set_status(rb_channel_t *chl, int status)
{
    chl->status = status;
}

void
rb_chl_set_flag(rb_channel_t *chl, int flag)
{
    chl->flag |= flag;
}

void
rb_chl_clear_flag(rb_channel_t *chl, int flag)
{
    chl->flag &= ~flag;
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
     * ev.revents被设置为当前已关注的所有事件 */
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
    rb_chl_set_status(chl, RB_CONNECTED);
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
    if (rb_chl_is_sending(chl)) {
        rb_chl_set_write_complete_cb(chl, rb_chl_del);
    } else {
        int fd = chl->ev.ident;
        rb_chl_set_status(chl, RB_CLOSED);
        chl->loop->evsel->remove(chl->loop->evop, fd);
        rb_hash_delete(chl->loop->chlist, fd);
        close(fd);
    }
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
    rb_buffer_destroy(&chl->input);
    rb_buffer_destroy(&chl->output);
    if (chl->user.data)
        chl->user.dealloc(chl->user.data);
    rb_free(chl);
}
