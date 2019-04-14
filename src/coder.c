#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "buffer.h"
#include "vector.h"
#include "coder.h"
#include "channel.h"

/*
 * 获取消息长度
 */
static size_t
rb_get_msglen(rb_buffer_t *b)
{
    return atol(rb_buffer_begin(b));
}

/*
 * 将[char *]打包成[buffer]，添加消息长度字段。
 */
void
rb_pack_add_len(rb_buffer_t *b, const char *s, size_t len)
{
    char msglen[RB_MSG_FIELD_SZ] = { 0 };

    snprintf(msglen, RB_MSG_FIELD_SZ - 1, "%zu", len);
    rb_buffer_write(b, msglen, RB_MSG_FIELD_SZ);
    rb_buffer_write(b, s, len);
}

/*
 * 从[buffer]中解析消息，有消息长度字段的格式。
 */
void
rb_unpack_with_len(rb_channel_t *chl)
{
    /* 可能有一条完整的消息 */
    while (rb_buffer_readable(chl->input) >= RB_MSG_FIELD_SZ) {
        size_t len = rb_get_msglen(chl->input);
        /* 如果能构造成一条完整的消息，就循环接收，直到不能构造成
         * 一条完整的消息。
         */
        ssize_t readable = rb_buffer_readable(chl->input);
        if (readable >= len + RB_MSG_FIELD_SZ) {
            rb_buffer_update_readidx(chl->input, RB_MSG_FIELD_SZ);
            if (chl->msgcb)
                chl->msgcb(chl, len);
            /* 如果没读完这条消息，就丢掉剩余部分 */
            if (rb_buffer_readable(chl->input) + len > readable)
                rb_buffer_update_readidx(chl->input,
                        rb_buffer_readable(chl->input) - len);
        } else
            /* 如果没有一条完整的消息，就直接跳出循环，等待从客户端接受到一条
             * 完整的消息。
             */
            break;
    }
}

/*
 * 返回[\r\n]在[buffer]中第一次出现的位置。
 */
static int
rb_find_crlf(rb_buffer_t *b)
{
    char *s = rb_buffer_begin(b);
    char *p = strnstr(s, RB_CRLF, rb_buffer_readable(b));
    return p ? p - s : -1;
}

/*
 * 从[buffer]中解析消息，以[\r\n]分隔。
 */
void
rb_unpack_with_crlf(rb_channel_t *chl)
{
    while (rb_buffer_readable(chl->input) >= RB_CRLF_LEN) {
        int crlf = rb_find_crlf(chl->input);
        size_t readable = rb_buffer_readable(chl->input);
        /* 找到了[\r\n]就读取一行文本，否则就退出循环 */
        if (crlf >= 0) {
            if (chl->msgcb)
                chl->msgcb(chl, crlf);
            if (rb_buffer_readable(chl->input) + crlf > readable)
                rb_buffer_update_readidx(chl->input,
                        rb_buffer_readable(chl->input) - crlf);
            rb_buffer_update_readidx(chl->input, RB_CRLF_LEN);
        } else
            break;
    }
}

/*
 * 将[buffer]中的数据发送出去
 */
void
rb_send(rb_channel_t *chl, rb_buffer_t *b)
{
    size_t readable = rb_buffer_readable(b);
    if (!rb_chl_is_writing(chl) && rb_buffer_readable(chl->output) == 0) {
        ssize_t n = write(chl->ev.ident, rb_buffer_begin(b), readable);
        if (n > 0) {
            if (n < readable) {
                /* 没写完，将剩余的数据添加到[buffer]中，并注册EV_WRITE事件，
                 * 当fd变得可写时，再继续发送。
                 */
                rb_buffer_write(chl->output, rb_buffer_begin(b) + n, readable - n);
                rb_buffer_update_readidx(chl->output, readable - n);
                rb_chl_enable_write(chl);
            }
        }
    } else {
        /* 如果有新到来的消息时，[buffer]中还有未发完的数据，就将
         * 新到来的消息追加到[buffer]的末尾，之后统一发送。
         * (这样才能保证接收方接收到消息的正确性)。
         */
        rb_buffer_move_forward(chl->output, readable);
        rb_buffer_write(chl->output, rb_buffer_begin(b), readable);
        rb_buffer_update_readidx(chl->output, readable);
    }
}
