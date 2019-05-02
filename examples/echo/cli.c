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
stdincb(rb_channel_t *in, rb_channel_t *out)
{
    char *s = rb_buffer_begin(in->input);
    size_t len = rb_buffer_readable(in->input);
    rb_buffer_retrieve(in->input, len);
    rb_send(out, s, len);
}

int
main(void)
{
    rb_user_t user = { 0 };
    rb_cli_t *cli = rb_cli_init();
    rb_cli_connect(cli, 6000, "127.0.0.1", msgcb, NULL, stdincb, user);
    rb_cli_run(cli);
}
