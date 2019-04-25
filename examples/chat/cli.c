#include "../usr_cli.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static void
msgcb(rb_channel_t *chl)
{
    char *s = rb_buffer_begin(chl->input);
    size_t len = rb_buffer_readable(chl->input);
    rb_buffer_retrieve(chl->input, len);
    printf("%s", s);
}

static void
concb(rb_channel_t *chl)
{
    char *s = "Hello, everyone!\n";
    rb_buffer_t *b = rb_buffer_init();
    rb_buffer_write(b, s, strlen(s));
    chat_add_msglen(b, strlen(s));

    char *buf = rb_buffer_begin(b);
    size_t len = rb_buffer_readable(b);
    rb_send(chl, buf, len);
    rb_buffer_destroy(&b);
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init(6001, "127.0.0.1");
    rb_cli_set_cb(cli, msgcb, concb);
    rb_cli_run(cli);
}
