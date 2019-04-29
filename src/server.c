#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include "alloc.h"
#include "server.h"
#include "evll.h"
#include "evthr.h"
#include "evloop.h"
#include "channel.h"
#include "net.h"
#include "lock.h"
#include "queue.h"
#include "log.h"

rb_serv_t *
rb_serv_init(int loops)
{
    rb_serv_t *serv = rb_malloc(sizeof(rb_serv_t));

    serv->evll = rb_evll_init(loops);
    rb_evloop_t *loop = rb_evloop_init();
    serv->mloop = *loop;
    rb_free(loop);

    return serv;
}

/*
 * 接收一个[connfd]，然后分发给[evll]中的一个[loop]
 */
static void
rb_serv_accept(rb_channel_t *_chl)
{
    rb_serv_t *serv = ((rb_serv_t *)((char *)_chl->loop - offsetof(rb_serv_t, mloop)));
    rb_evthr_t *evthr = rb_evll_get_nextloop(serv->evll);
    rb_evloop_t *loop = evthr->loop;

    rb_channel_t *chl = rb_chl_init(loop);
    chl->ev.ident = rb_accept(_chl->ev.ident);
    rb_chl_set_status(chl, RB_CONNECTING);
    rb_chl_set_cb(chl, rb_handle_event, rb_handle_read, rb_handle_write,
            rb_handle_close, _chl->msgcb);
    if (_chl->acpcb)
        _chl->acpcb(chl);
    if (rb_chl_is_connecting(chl))
        rb_chl_add(chl);
}

/*
 * 让一个server可以同时监听多个port
 */
void
rb_serv_listen_ports(rb_serv_t *serv, int ports[], size_t len,
        void (*msgcb[])(rb_channel_t *),
        void (*acpcb[])(rb_channel_t *))
{
    for (int i = 0; i < len; i++) {
        rb_channel_t *chl = rb_chl_init(&serv->mloop);
        chl->ev.ident = rb_listen(ports[i]);
        rb_chl_set_cb(chl, rb_handle_event, rb_serv_accept, NULL,
                rb_handle_close, msgcb[i]);
        rb_chl_set_acpcb(chl, acpcb[i]);
        rb_chl_add(chl);
    }
}

/*
 * 通常我们只需要监听一个port，这时使用这个接口比较方便一点
 */
void
rb_serv_listen(rb_serv_t *serv, int port, void (*msgcb)(rb_channel_t *),
        void (*acpcb)(rb_channel_t *))
{
    void (*mcb[])(rb_channel_t *) = { msgcb };
    void (*acb[])(rb_channel_t *) = { acpcb };
    rb_serv_listen_ports(serv, &port, 1, mcb, acb);
}

void
rb_serv_run(rb_serv_t *serv)
{
    rb_evll_run(serv->evll);
    rb_evloop_run(&serv->mloop);
    rb_free(serv);
}

void
rb_serv_quit(rb_serv_t *serv)
{
    rb_evll_quit(serv->evll);
    rb_free(serv->evll);
    rb_evloop_quit(&serv->mloop);
}
