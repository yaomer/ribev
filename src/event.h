#ifndef _RIBEV_EVENT_H
#define _RIBEV_EVENT_H

#include <sys/event.h>

enum RB_EV_TYPE {
    RB_EV_READ  = EVFILT_READ,
    RB_EV_WRITE = EVFILT_WRITE, 
    RB_EV_ERROR = EVFILT_EXCEPT,
};

typedef struct rb_event {
    int ident;
    int events;
    int revents;
} rb_event_t;

#define rb_ev_set(ev, id, evs, revs) \
    do { (ev)->ident = (id); \
         (ev)->events = (evs); \ 
         (ev)->revents = (revs) \
    } while (0)

#endif /* _RIBEV_EVENT_H */
