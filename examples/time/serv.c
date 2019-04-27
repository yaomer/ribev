/*
 * time协议：以32位整数传递时间
 */

#include "../usr_serv.h"
#include <stdio.h>
#include <string.h>

static void
msgcb(rb_channel_t *chl)
{
    size_t len = rb_buffer_readable(chl->input);
    rb_buffer_retrieve(chl->input, len);

    int32_t tm = (int32_t)time(NULL);
    rb_send(chl, (void *)&tm, sizeof(tm));
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 6003, msgcb);
    rb_serv_run(serv);
}
