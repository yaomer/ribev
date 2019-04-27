/*
 * daytime协议：以ASCII字符串传递时间
 */

#include "../usr_serv.h"
#include <string.h>

static void
msgcb(rb_channel_t *chl)
{
    /* 丢弃读到的数据 */
    size_t len = rb_buffer_readable(chl->input);
    rb_buffer_retrieve(chl->input, len);

    char buf[32];
    bzero(buf, sizeof(buf));
    /* rb_get_timestr(rb_now(), buf, sizeof(buf)); */
    rb_local_timestr(buf, sizeof(buf));
    len = strlen(buf);
    rb_send(chl, buf, len);
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 6002, msgcb);
    rb_serv_run(serv);
}
