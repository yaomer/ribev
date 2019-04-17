#ifndef _RIBEV_CLIENT_H
#define _RIBEV_CLIENT_H

#include <stddef.h>
#include "fwd.h"

typedef struct rb_cli {
    rb_evloop_t *loop;
    int port;
    char *addr;
    void (*msgcb)(rb_channel_t *, size_t);
    void (*concb)(rb_channel_t *);
} rb_cli_t;

#define rb_cli_set_cb(cli, msg, con) \
    do { \
        (cli)->msgcb = msg; \
        (cli)->concb = con; \
    } while (0)

rb_cli_t *rb_cli_init(int port, char *addr);
void rb_cli_run(rb_cli_t *cli);

#endif /* _RIBEV_CLIENT_H */
