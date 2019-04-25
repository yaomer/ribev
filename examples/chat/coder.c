#include "../usr_cli.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * 添加消息长度字段
 */
void
chat_add_msglen(rb_buffer_t *b, size_t len)
{
    b->readindex -= MSG_FIELD_SZ;
    rb_vector_resize(b->buf, b->readindex);
    char *buf = rb_buffer_begin(b);
    snprintf(buf, MSG_FIELD_SZ - 1, "%zu", len);
    buf[MSG_FIELD_SZ - 1] = '\0';
}

/*
 * 返回消息长度
 */
size_t
chat_get_msglen(rb_buffer_t *b)
{
    return atol(rb_buffer_begin(b));
}
