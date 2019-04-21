#ifndef _RIBEV_CHANNEL_H
#define _RIBEV_CHANNEL_H

#include "fwd.h"
#include "event.h"

typedef struct rb_channel {
    rb_evloop_t *loop;
    rb_event_t ev;
    rb_buffer_t *input;
    rb_buffer_t *output;
    void (*eventcb)(rb_channel_t *);
    void (*readcb)(rb_channel_t *);
    void (*writecb)(rb_channel_t *);
    void (*msgcb)(rb_channel_t *);
} rb_channel_t;

#define rb_chl_set_cb(chl, e, r, w, m) \
    do { \
        (chl)->eventcb = e; \
        (chl)->readcb = r; \
        (chl)->writecb = w; \
        (chl)->msgcb = m; \
    } while (0)

rb_channel_t *rb_chl_init(rb_evloop_t *loop);
void rb_chl_add(rb_channel_t *chl);
void rb_chl_del(rb_channel_t *chl);
int rb_chl_is_reading(rb_channel_t *chl);
int rb_chl_is_writing(rb_channel_t *chl);
void rb_chl_enable_read(rb_channel_t *chl);
void rb_chl_enable_write(rb_channel_t *chl);
void rb_chl_disable_read(rb_channel_t *chl);
void rb_chl_disable_write(rb_channel_t *chl);
void rb_free_chl(void *arg);

#endif /* _RIBEV_CHANNEL_H */
