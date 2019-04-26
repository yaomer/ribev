#include <stdio.h>
#include "alloc.h"
#include "client.h"
#include "evloop.h"
#include "channel.h"
#include "buffer.h"
#include "net.h"
#include "log.h"

rb_cli_t *
rb_cli_init(int port, char *addr)
{
    rb_cli_t *cli = rb_malloc(sizeof(rb_cli_t));

    cli->port = port;
    cli->addr = addr;
    cli->chl = NULL;
    rb_evloop_t *loop = rb_evloop_init();
    cli->loop = *loop;
    rb_free(loop);
    rb_cli_set_cb(cli, NULL, NULL, NULL);

    return cli;
}

static void
rb_cli_destroy(rb_cli_t **_cli)
{
    rb_free(*_cli);
    *_cli = NULL;
}

static void
rb_read_stdin(rb_channel_t *chl)
{
    int err;
    ssize_t n = rb_read_fd(chl->input, chl->ev.ident, &err);
    rb_cli_t *cli = ((rb_cli_t *)((char *)chl->loop - offsetof(rb_cli_t, loop)));

    if (n > 0) {
        if (cli->stdincb)
            cli->stdincb(chl, cli->chl);
    } else if (n == 0)
        rb_cli_close(chl);
    else
        rb_log_error("stdin error");
}

/*
 * 虽说server一般不和stdin打交道，但client却要经常用到stdin，
 * 为了方便起见，我们将stdin也融入到[event loop]中
 */
static void
rb_cli_set_stdin(rb_cli_t *cli)
{
    rb_channel_t *in = rb_chl_init(&cli->loop);
    in->ev.ident = 0;
    rb_chl_set_cb(in, rb_handle_event, rb_read_stdin, NULL, NULL);
    rb_chl_add(in);
}

void
rb_cli_run(rb_cli_t *cli)
{
    cli->chl = rb_chl_init(&cli->loop);
    cli->chl->ev.ident = rb_connect(cli->port, cli->addr);
    rb_chl_set_cb(cli->chl, rb_handle_event, rb_handle_read, rb_handle_write,
            cli->msgcb);
    rb_chl_add(cli->chl);
    rb_cli_set_stdin(cli);
    if (cli->concb)
        cli->concb(cli->chl);
    rb_evloop_run(&cli->loop);
    rb_cli_destroy(&cli);
}

void
rb_cli_close(rb_channel_t *chl)
{
    rb_evloop_quit(chl->loop);
}
