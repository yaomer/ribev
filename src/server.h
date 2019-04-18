#ifndef _RIBEV_SERVER_H
#define _RIBEV_SERVER_H

#include "fwd.h"
#include "evloop.h"

typedef struct rb_serv {
    int port;
    rb_evloop_t mloop;
    rb_evll_t *evll;
    void (*msgcb)(rb_channel_t *);
} rb_serv_t;

#define rb_serv_set_cb(serv, m) \
    do { \
        (serv)->msgcb = m; \
    } while (0)

rb_serv_t *rb_serv_init(int loops, int port);
void rb_serv_run(rb_serv_t *serv);

#endif /* _RIBEV_SERVER_H */
