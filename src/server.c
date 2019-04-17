#include <stddef.h>
#include "alloc.h"
#include "server.h"
#include "evll.h"
#include "evthr.h"
#include "evloop.h"
#include "channel.h"
#include "coder.h"
#include "net.h"
#include "lock.h"
#include "queue.h"

rb_serv_t *
rb_serv_init(int loops, int port)
{
    rb_serv_t *serv = rb_malloc(sizeof(rb_serv_t));

    serv->port = port;
    serv->evll = rb_evll_init(loops);
    rb_evloop_t *loop = rb_evloop_init();
    serv->mloop = *loop;
    rb_free(loop);
    rb_serv_set_cb(serv, NULL);

    return serv;
}

/*
 * 接收一个[connfd]，然后分发给[evll]中的一个[loop]
 */
static void
rb_serv_accept(rb_channel_t *chl)
{
    rb_serv_t *serv = ((rb_serv_t *)((char *)chl->loop - offsetof(rb_serv_t, mloop)));
    rb_evthr_t *evthr = rb_evll_get_nextloop(serv->evll);
    rb_evloop_t *loop = evthr->loop;

    rb_channel_t *ch = rb_chl_init(loop);
    ch->ev.ident = rb_accept(chl->ev.ident);
    rb_chl_set_cb(ch, rb_handle_event, rb_handle_read, rb_handle_write,
            serv->msgcb, rb_pack_add_len, rb_unpack_with_len);

    rb_lock(&loop->mutex);
    rb_queue_push(loop->ready_chls, ch);
    rb_wakeup(loop);
    rb_unlock(&loop->mutex);
}

void
rb_serv_run(rb_serv_t *serv)
{
    rb_channel_t *chl = rb_chl_init(&serv->mloop);
    chl->ev.ident = rb_listen(serv->port);
    rb_chl_set_cb(chl, rb_handle_event, rb_serv_accept, NULL, NULL, NULL, NULL);
    rb_chl_add(chl);
    rb_evll_run(serv->evll);
    rb_evloop_run(&serv->mloop);
}
