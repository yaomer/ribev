#include "../usr_cli.h"
#include <stdio.h>
#include <string.h>

static void
msgcb(rb_channel_t *chl)
{
    size_t len = rb_buffer_readable(chl->input);
    char buf[len + 1];
    bzero(buf, sizeof(buf));
    rb_buffer_read(chl->input, buf, len + 1);
    printf("%s", buf);
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init();
    rb_cli_connect(cli, 6002, "127.0.0.1", msgcb, NULL, NULL);
    rb_cli_run(cli);
}
