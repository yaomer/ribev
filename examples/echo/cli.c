#include "../usr_cli.h"
#include <stdio.h>
#include <string.h>

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
    char *s = rb_buffer_begin(from->input);
    size_t len = rb_buffer_readable(from->input);
    rb_buffer_retrieve(from->input, len);
    rb_send(to, s, len);
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init(6000, "127.0.0.1");
    rb_cli_set_cb(cli, msgcb, NULL, stdincb);
    rb_cli_run(cli);
}
