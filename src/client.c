#include "alloc.h"
#include "client.h"
#include "evloop.h"
#include "channel.h"
#include "net.h"

rb_cli_t *
rb_cli_init(int port, char *addr)
{
    rb_cli_t *cli = rb_malloc(sizeof(rb_cli_t));

    cli->port = port;
    cli->addr = addr;
    cli->loop = rb_evloop_init();
    rb_cli_set_cb(cli, NULL, NULL);

    return cli;
}

void
rb_cli_run(rb_cli_t *cli)
{
    rb_channel_t *chl = rb_chl_init(cli->loop);
    chl->ev.ident = rb_connect(cli->port, cli->addr);
    rb_chl_set_cb(chl, rb_handle_event, rb_handle_read, rb_handle_write,
            cli->msgcb);
    rb_chl_add(chl);
    if (cli->concb)
        cli->concb(chl);
    rb_evloop_run(cli->loop);
}

void
rb_cli_close(rb_channel_t *chl)
{
    rb_evloop_quit(chl->loop);
}
