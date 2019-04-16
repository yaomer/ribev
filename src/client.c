#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "evloop.h"
#include "channel.h"
#include "event.h"
#include "net.h"
#include "coder.h"
#include "buffer.h"

void
msg(rb_channel_t *chl, size_t len)
{
    char s[len + 1];
    rb_buffer_t *b = rb_buffer_init();
    rb_buffer_read(chl->input, s, len + 1);
    chl->packcb(b, s, strlen(s));
    rb_send(chl, b);
    rb_buffer_destroy(&b);
}

void *
f(void *a)
{
    rb_evloop_t *l = rb_evloop_init();
    rb_channel_t *ch = rb_chl_init(l);
    ch->ev.ident = rb_connect(6000, "127.0.0.1");
    rb_chl_set_cb(ch, rb_handle_event, rb_handle_read, rb_handle_write, msg, rb_pack_add_len, rb_unpack_with_len);
    rb_chl_add(ch);
    rb_chl_enable_read(ch);

    rb_buffer_t *b = rb_buffer_init();
    char s[10240];
    memset(s, 'c', 10240);
    s[10238] = '\n';
    s[10239] = '\0';
    ch->packcb(b, s, 10239);
    rb_send(ch, b);
    rb_evloop_run(l);
    return 0;
}

int
main(int argc, char *argv[])
{
    int n = atoi(argv[1]);
    pthread_t tid[n];
    for (int i = 0; i < n; i++)
        pthread_create(&tid[i], NULL, f, NULL);
    while (1)
        ;
}
