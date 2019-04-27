#ifndef _RIBEV_CLIENT_H
#define _RIBEV_CLIENT_H

#include <stddef.h>
#include "evloop.h"
#include "fwd.h"

typedef struct rb_cli {
    rb_evloop_t loop;
    rb_channel_t *chl;
    void (*stdincb)(rb_channel_t *, rb_channel_t *);
} rb_cli_t;

void
rb_cli_connect_s(rb_cli_t *cli, int ports[], char *addr[], size_t len,
        void (*msgcb[])(rb_channel_t *),
        void (*concb[])(rb_channel_t *),
        size_t index,
        void (*stdincb)(rb_channel_t *, rb_channel_t *));
void rb_cli_connect(rb_cli_t *cli, int port, char *addr,
        void (*msgcb)(rb_channel_t *),
        void (*concb)(rb_channel_t *),
        void (*stdincb)(rb_channel_t *, rb_channel_t *));

rb_cli_t *rb_cli_init(void);
void rb_cli_run(rb_cli_t *cli);
void rb_cli_close(rb_channel_t *chl);

#endif /* _RIBEV_CLIENT_H */
