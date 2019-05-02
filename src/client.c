#include <stdio.h>
#include "alloc.h"
#include "client.h"
#include "evloop.h"
#include "channel.h"
#include "buffer.h"
#include "user.h"
#include "net.h"
#include "log.h"

rb_cli_t *
rb_cli_init(void)
{
    rb_cli_t *cli = rb_malloc(sizeof(rb_cli_t));

    rb_evloop_t *loop = rb_evloop_init();
    cli->loop = *loop;
    rb_free(loop);
    cli->chl = NULL;

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
    ssize_t n = rb_read_fd(chl->input, chl->ev.ident);
    rb_cli_t *cli = ((rb_cli_t *)((char *)chl->loop - offsetof(rb_cli_t, loop)));

    if (n > 0) {
        if (cli->stdincb)
            cli->stdincb(chl, cli->chl);
    } else if (n == 0)
        chl->closecb(chl);
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
    rb_user_t user;
    user.init = NULL;
    rb_channel_t *in = rb_chl_init(&cli->loop, user);
    in->ev.ident = 0;
    rb_chl_set_cb(in, rb_handle_event, rb_read_stdin, NULL,
            rb_cli_close, NULL);
    rb_chl_add(in);
}

/*
 * stdin只能绑定到一个chl上
 */
void
rb_cli_connect_s(rb_cli_t *cli, int ports[], char *addr[], size_t len,
        void (*msgcb[])(rb_channel_t *),
        void (*concb[])(rb_channel_t *),
        size_t index,  /* 将stdin绑定到连接ports[index]的chl上 */
        void (*stdincb)(rb_channel_t *, rb_channel_t *),
        rb_user_t user[])
{
    cli->stdincb = stdincb;
    for (int i = 0; i < len; i++) {
        rb_channel_t *chl = rb_chl_init(&cli->loop, user[i]);
        chl->ev.ident = rb_connect(ports[i], addr[i]);
        rb_chl_set_cb(chl, rb_handle_event, rb_handle_read, rb_handle_write,
                rb_cli_close, msgcb[i]);
        rb_chl_set_concb(chl, concb[i]);
        rb_chl_add(chl);
        if (chl->concb)
            chl->concb(chl);
        if (i == index) {
            rb_cli_set_stdin(cli);
            cli->chl = chl;
        }
    }
}

void
rb_cli_connect(rb_cli_t *cli, int port, char *addr,
        void (*msgcb)(rb_channel_t *),
        void (*concb)(rb_channel_t *),
        void (*stdincb)(rb_channel_t *, rb_channel_t *),
        rb_user_t user)
{
    void (*mcb[])(rb_channel_t *) = { msgcb };
    void (*ccb[])(rb_channel_t *) = { concb };
    rb_cli_connect_s(cli, &port, &addr, 1, mcb, ccb, 0, stdincb, &user);
}

void
rb_cli_run(rb_cli_t *cli)
{
    rb_evloop_run(&cli->loop);
    rb_cli_destroy(&cli);
}

void
rb_cli_close(rb_channel_t *chl)
{
    if (rb_chlist_size(chl->loop) > 3)
        rb_handle_close(chl);
    else
        rb_evloop_quit(chl->loop);
}
