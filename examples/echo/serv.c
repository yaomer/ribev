#include "../usr_serv.h"
#include <stdio.h>
#include <string.h>

static void
msgcb(rb_channel_t *chl)
{
    char *s = rb_buffer_begin(chl->input);
    size_t len = rb_buffer_readable(chl->input);
    rb_send(chl, s, len);
    rb_buffer_retrieve(chl->input, len);
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1);
    rb_serv_listen(serv, 6000, msgcb);
    rb_serv_run(serv);
}
