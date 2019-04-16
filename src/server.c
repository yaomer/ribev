#include <stdio.h>
#include "evloop.h"
#include "channel.h"
#include "event.h"
#include "net.h"
#include "coder.h"
#include "buffer.h"
#include "task.h"
#include "timer.h"
#include "log.h"

void
msg(rb_channel_t *chl, size_t len)
{
    char s[len + 1];
    rb_buffer_read(chl->input, s, len + 1);

    rb_buffer_t *b = rb_buffer_init();
    chl->packcb(b, s, len);
    rb_send(chl, b);
    rb_buffer_destroy(&b);
    static int i = 0;
    printf("%.2f\n", ++i * len * 1.0 / 1000000);
}

int listenfd;

void
acp(rb_channel_t *chl)
{
    if (chl->ev.ident == listenfd) {
        rb_channel_t *ch = rb_chl_init(chl->loop);
        ch->ev.ident = rb_accept(chl->ev.ident);
        rb_chl_set_cb(ch, rb_handle_event, rb_handle_read, rb_handle_write, msg, rb_pack_add_len, rb_unpack_with_len);
        rb_chl_add(ch);
        rb_chl_enable_read(ch);
        printf("%d connected\n", ch->ev.ident);
    } else {
        rb_handle_read(chl);
    }
}

void
xp(void **x)
{
    printf("hello, world\n");
}

int
main(void)
{
    rb_log_init();
    rb_evloop_t *l = rb_evloop_init();
    rb_channel_t *ch = rb_chl_init(l);
    ch->ev.ident = rb_listen(6000);
    listenfd = ch->ev.ident;
    printf("listenfd %d\n", listenfd);
    rb_chl_set_cb(ch, rb_handle_event, acp, rb_handle_write, msg, rb_pack_add_len, rb_unpack_with_len);
    rb_chl_add(ch);
    rb_chl_enable_read(ch);

    /* rb_task_t *t = rb_alloc_task(0);
    t->callback = xp;
    rb_run_every(l, 100, t); */
    rb_evloop_run(l);
}
