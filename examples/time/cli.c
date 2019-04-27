#include "../usr_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void
msgcb(rb_channel_t *chl)
{
    if (rb_buffer_readable(chl->input) >= sizeof(int32_t)) {
        char buf[32];
        bzero(buf, sizeof(buf));
        size_t len = rb_buffer_readable(chl->input);
        rb_buffer_read(chl->input, buf, len + 1);
        int32_t tm32 = *(int32_t *)buf;
        time_t tm = (time_t)tm32;
        printf("%s", ctime_r(&tm, buf));
        rb_cli_close(chl);
    }
}

static void
concb(rb_channel_t *chl)
{
    /* 唤醒server */
    rb_send(chl, "x", 1);
}

int
main(void)
{
    rb_cli_t *cli = rb_cli_init();
    rb_cli_connect(cli, 6003, "127.0.0.1", msgcb, concb, NULL);
    rb_cli_run(cli);
}
