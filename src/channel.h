#ifndef _RIBEV_CHANNEL_H
#define _RIBEV_CHANNEL_H

#include "fwd.h"

typedef struct rb_channel {
    rb_evloop_t *loop;
    rb_event_t ev;
    rb_buffer_t *input;
    rb_buffer_t *output;
    void (*eventcb)(rb_channel_t *);
    void (*readcb)(rb_channel_t *);
    void (*writecb)(rb_channel_t *);
    void (*msgcb)(rb_channel_t *, size_t);
    void (*packcb)(rb_buffer_t *, const char *, size_t);
    void (*unpackcb)(rb_channel_t *);
} rb_channel_t;

#define rb_chl_set_eventcb(chl, cb) ((chl)->eventcb = cb)
#define rb_chl_set_readcb(chl, cb) ((chl)->readcb = cb)
#define rb_chl_set_writecb(chl, cb) ((chl)->writecb = cb)
#define rb_chl_set_msgcb(chl, cb) ((chl)->msgcb = cb)
#define rb_chl_set_packcb(chl, cb) ((chl)->packcb = cb)
#define rb_chl_set_unpackcb(chl, cb) ((chl)->unpackcb = cb)

rb_channel_t *rb_chl_init(rb_evloop_t *loop);
int rb_chl_is_reading(rb_channel_t *chl);
int rb_chl_is_writing(rb_channel_t *chl);
void rb_chl_enable_read(rb_channel_t *chl);
void rb_chl_enable_write(rb_channel_t *chl);
void rb_chl_disable_read(rb_channel_t *chl);
void rb_chl_disable_write(rb_channel_t *chl);

#endif /* _RIBEV_CHANNEL_H */
