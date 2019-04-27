#include "../usr_cli.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static void
msgcb(rb_channel_t *chl)
{
    size_t len = rb_buffer_readable(chl->input);
    char s[len + 1];
    bzero(s, sizeof(s));
    rb_buffer_read(chl->input, s, len + 1);
    rb_buffer_retrieve(chl->input, len);
    printf("%s", s);
}

static void
stdincb(rb_channel_t *from, rb_channel_t *to)
{
    chat_add_msglen(from->input, rb_buffer_readable(from->input));
    char *s = rb_buffer_begin(from->input);
    size_t len = rb_buffer_readable(from->input);
    rb_buffer_retrieve(from->input, len);
    rb_send(to, s, len);
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init();
    rb_cli_connect(cli, 6001, "127.0.0.1", msgcb, NULL, stdincb);
    rb_cli_run(cli);
}
