/*
 * time协议：以32位整数传递时间
 */

#include "../usr_serv.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void
write_complete_cb(rb_channel_t *chl)
{
    close(chl->ev.ident);
    rb_free_chl(chl);
}

static void
acpcb(rb_channel_t *chl)
{
    int32_t tm = (int32_t)time(NULL);
    rb_send(chl, (void *)&tm, sizeof(tm));
    rb_chl_set_status(chl, RB_CLOSED);
    rb_chl_set_write_complete_cb(chl, write_complete_cb);
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 6003, NULL, acpcb);
    rb_serv_run(serv);
}
