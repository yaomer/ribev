#ifndef _RIBEV_CLIENT_H
#define _RIBEV_CLIENT_H

#include <stddef.h>
#include "evloop.h"
#include "fwd.h"

typedef struct rb_cli {
    int port;
    char *addr;
    rb_evloop_t loop;
    rb_channel_t *chl;
    void (*msgcb)(rb_channel_t *);
    void (*concb)(rb_channel_t *);
    void (*stdincb)(rb_channel_t *, rb_channel_t *);
} rb_cli_t;

#define rb_cli_set_cb(cli, m, c, s) \
    do { \
        (cli)->msgcb = m; \
        (cli)->concb = c; \
        (cli)->stdincb = s; \
    } while (0)

rb_cli_t *rb_cli_init(int port, char *addr);
void rb_cli_run(rb_cli_t *cli);
void rb_cli_close(rb_channel_t *chl);

#endif /* _RIBEV_CLIENT_H */
