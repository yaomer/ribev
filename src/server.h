#ifndef _RIBEV_SERVER_H
#define _RIBEV_SERVER_H

#include "fwd.h"
#include "evloop.h"

typedef struct rb_serv {
    rb_evloop_t mloop;
    rb_evll_t *evll;
} rb_serv_t;

void rb_serv_listen_ports(rb_serv_t *serv, int ports[], size_t len, 
        void (*msgcb[])(rb_channel_t *),
        void (*acpcb[])(rb_channel_t *));
void rb_serv_listen(rb_serv_t *serv, int port, void (*msgcb)(rb_channel_t *),
        void (*acpcb)(rb_channel_t *));

rb_serv_t *rb_serv_init(int loops);
void rb_serv_run(rb_serv_t *serv);
void rb_serv_quit(rb_serv_t *serv);

#endif /* _RIBEV_SERVER_H */
