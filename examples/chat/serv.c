/*
 * 一个简单的聊天服务：
 * server将某个client发来的消息转发给所有在线的client
 */

#include "../usr_serv.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>

/*
 * 将消息转发给所有在线的client
 */
static void
chat_deal_msg(rb_channel_t *chl, size_t len)
{
    rb_hash_t *clients = chl->loop->chlist;
    char *buf = rb_buffer_begin(chl->input);

    for (int i = 0; i < clients->hashsize; i++) {
        struct hash_node *h = clients->buckets[i];
        while (h) {
            rb_channel_t *ch = (rb_channel_t *)h->data;
            rb_send(ch, buf, len);
            h = h->next;
        }
    }
    rb_buffer_retrieve(chl->input, len);
}

/*
 * 简单的消息分包示例
 */
static void
msgcb(rb_channel_t *chl)
{
    while (rb_buffer_readable(chl->input) >= MSG_FIELD_SZ) {
        size_t len = chat_get_msglen(chl->input);
        /* 能构成一条完整的消息 */
        if (rb_buffer_readable(chl->input) >= len + MSG_FIELD_SZ) {
            rb_buffer_retrieve(chl->input, MSG_FIELD_SZ);
            chat_deal_msg(chl, len);
        } else
            break;
    }
}

int
main(void)
{
    rb_serv_t *serv = rb_serv_init(1, 6001);
    rb_serv_set_cb(serv, msgcb);
    rb_serv_run(serv);
}
