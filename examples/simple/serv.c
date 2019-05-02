/*
 * 三合一
 * 一个server提供time、daytime和echo服务
 */

#include "../usr_serv.h"
#include <string.h>

static void
_time(rb_channel_t *chl)
{
    int32_t tm = (int32_t)time(NULL);
    rb_send(chl, (void *)&tm, sizeof(tm));
    chl->closecb(chl);
}

static void
_daytime(rb_channel_t *chl)
{
    char buf[32];
    bzero(buf, sizeof(buf));
    rb_local_timestr(buf, sizeof(buf));
    rb_send(chl, buf, strlen(buf));
    chl->closecb(chl);
}

static void
_echo(rb_channel_t *chl)
{
    char *s = rb_buffer_begin(chl->input);
    size_t len = rb_buffer_readable(chl->input);
    rb_send(chl, s, len);
    rb_buffer_retrieve(chl->input, len);
}

int
main(void)
{
    int ports[] = { 6003, 6002, 6000 };
    void (*msgcb[])(rb_channel_t *) = { NULL, NULL, _echo };
    void (*acpcb[])(rb_channel_t *) = { _time, _daytime, NULL };
    rb_user_t user[3] = { {0}, {0}, {0} };
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen_ports(serv, ports, 3, msgcb, acpcb, user);
    rb_serv_run(serv);
}
