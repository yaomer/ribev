#ifndef _RIBEV_CODER_H
#define _RIBEV_CODER_H

/*
 * 我们支持两种[coder]方式，一种是在消息的头部添加长度字段，
 * 另一种是像[http]协议那样用CRLF作分隔符。
 */
/*
 * 消息长度字段的宽度
 */
#define RB_MSG_FIELD_SZ 8
/*
 * 消息分隔符
 */
#define RB_CRLF "\r\n"
#define RB_CRLF_LEN 2

void rb_pack_add_len(rb_buffer_t *b, const char *s, size_t len);
void rb_unpack_with_len(rb_channel_t *chl);
void rb_unpack_with_crlf(rb_channel_t *chl);
void rb_send(rb_channel_t *chl, rb_buffer_t *b);

#endif /* _RIBEV_CODER_H */
