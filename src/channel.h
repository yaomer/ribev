#ifndef _RIBEV_CHANNEL_H
#define _RIBEV_CHANNEL_H

#include <stdatomic.h>
#include "fwd.h"
#include "event.h"

/* status */
#define RB_CONNECTING   1   /* 连接已建立，但还未添加到loop中 */
#define RB_CONNECTED    2   /* 正常通信状态 */
#define RB_CLOSED       3   /* 断开连接 */
/* flag */
#define RB_SENDING  001     /* 消息正在发送 */

typedef struct rb_channel {
    atomic_int status;
    atomic_int flag;
    rb_evloop_t *loop;
    rb_event_t ev;
    rb_buffer_t *input;
    rb_buffer_t *output;
    /* 事件分发器 */
    void (*eventcb)(rb_channel_t *);
    void (*readcb)(rb_channel_t *);
    void (*writecb)(rb_channel_t *);
    void (*closecb)(rb_channel_t *);
    /* 双方正常通信时使用的回调 */
    void (*msgcb)(rb_channel_t *);
    /* client发起连接后调用的回调 */
    void (*concb)(rb_channel_t *);
    /* server接受连接后调用的回调 */
    void (*acpcb)(rb_channel_t *);
    /* 写完一条消息后触发的回调 */
    void (*write_complete_cb)(rb_channel_t *);
} rb_channel_t;

#define rb_chl_set_cb(chl, event, read, write, close, msg) \
    do { \
        (chl)->eventcb = event; \
        (chl)->readcb = read; \
        (chl)->writecb = write; \
        (chl)->closecb = close; \
        (chl)->msgcb = msg; \
    } while (0)

#define rb_chl_set_concb(chl, con) ((chl)->concb = con)
#define rb_chl_set_acpcb(chl, acp) ((chl)->acpcb = acp)
#define rb_chl_set_write_complete_cb(chl, wc) ((chl)->write_complete_cb = wc)

#define rb_chl_is_connecting(chl) ((chl)->status == RB_CONNECTING)
#define rb_chl_is_connected(chl) ((chl)->status == RB_CONNECTED)
#define rb_chl_is_closed(chl) ((chl)->status == RB_CLOSED)
#define rb_chl_is_reading(chl) ((chl)->ev.events & RB_EV_READ)
#define rb_chl_is_writing(chl) ((chl)->ev.events & RB_EV_WRITE)
#define rb_chl_is_sending(chl) ((chl)->flag & RB_SENDING)

rb_channel_t *rb_chl_init(rb_evloop_t *loop);
void rb_chl_set_status(rb_channel_t *chl, int status);
void rb_chl_set_flag(rb_channel_t *chl, int flag);
void rb_chl_clear_flag(rb_channel_t *chl, int flag);
void rb_chl_add(rb_channel_t *chl);
void rb_chl_del(rb_channel_t *chl);
void rb_chl_enable_read(rb_channel_t *chl);
void rb_chl_enable_write(rb_channel_t *chl);
void rb_chl_disable_read(rb_channel_t *chl);
void rb_chl_disable_write(rb_channel_t *chl);
void rb_free_chl(void *arg);

#endif /* _RIBEV_CHANNEL_H */
