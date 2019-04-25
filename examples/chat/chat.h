#ifndef _CHAT_CHAT_H
#define _CHAT_CHAT_H

#include <stddef.h>

#define MSG_FIELD_SZ RB_BUFFER_PREPEND

void chat_add_msglen(rb_buffer_t *b, size_t len);
size_t chat_get_msglen(rb_buffer_t *b);

#endif /* _CHAT_CHAT_H */
