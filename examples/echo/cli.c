#include "../usr_cli.h"
#include <stdio.h>
#include <string.h>

void
msgcb(rb_channel_t *chl)
{
    char *s = rb_buffer_begin(chl->input);
    size_t len = rb_buffer_readable(chl->input);
    rb_buffer_update_readidx(chl->input, len);
    printf("%s", s);
}

void
concb(rb_channel_t *chl)
{
    char *s = "ARE YOU OK?\n";
    rb_send(chl, s, strlen(s));
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init(6000, "127.0.0.1");
    rb_cli_set_cb(cli, msgcb, concb);
    rb_cli_run(cli);
}
