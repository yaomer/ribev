/*
 * daytime协议：以ASCII字符串传递时间
 */

#include "../usr_serv.h"
#include <string.h>
#include <unistd.h>

static void
acpcb(rb_channel_t *chl)
{
    char buf[32];
    bzero(buf, sizeof(buf));
    /* rb_get_timestr(rb_now(), buf, sizeof(buf)); */
    rb_local_timestr(buf, sizeof(buf));
    rb_send(chl, buf, strlen(buf));
    chl->closecb(chl);
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 6002, NULL, acpcb);
    rb_serv_run(serv);
}
