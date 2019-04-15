#ifndef _RIBEV_EVENT_H
#define _RIBEV_EVENT_H

#include <sys/event.h>
#include "fwd.h"

enum RB_EV_TYPE {
    RB_EV_READ  = 0x01,
    RB_EV_WRITE = 0x02, 
    RB_EV_ERROR = 0x04, 
};

extern const char *event_str[];

typedef struct rb_event {
    int ident;
    int events;
    int revents;
} rb_event_t;

#define rb_ev_set(ev, id, evs, revs) \
    do { \
        (ev)->ident = (id); \
        (ev)->events = (evs); \
        (ev)->revents = (revs); \
    } while (0)

void rb_handle_error(rb_channel_t *chl);
void rb_handle_close(rb_channel_t *chl);
void rb_handle_read(rb_channel_t *chl);
void rb_handle_write(rb_channel_t *chl);
void rb_handle_event(rb_channel_t *chl);

#endif /* _RIBEV_EVENT_H */
