#ifndef _RIBEV_BUFFER_H
#define _RIBEV_BUFFER_H

#include <sys/types.h>
#include "vector.h"
#include "fwd.h"

#define RB_BUFFER_PREPEND 8

/*
 *                [buffer]
 *  --------------------------------------
 * |  prepend  |  readable  |  writeable  |
 *  --------------------------------------
 *            ridx         widx
 */
typedef struct rb_buffer {
    rb_vector_t *buf;
    int readindex;
    int writeindex;
} rb_buffer_t;

#define rb_buffer_begin(b) \
    ((char *)(rb_vector_entry((b)->buf, 0) + (b)->readindex))
#define rb_buffer_prependable(b) ((b)->readindex)
#define rb_buffer_readable(b) ((b)->writeindex - (b)->readindex)
#define rb_buffer_writeable(b) (rb_vector_max_size((b)->buf) - (b)->writeindex)

rb_buffer_t * rb_buffer_init(void);
void rb_buffer_retrieve(rb_buffer_t *b, size_t len);
void rb_buffer_move_forward(rb_buffer_t *b, size_t len);
void rb_buffer_read(rb_buffer_t *b, char *s, size_t n);
void rb_buffer_write(rb_buffer_t *b, const char *s, size_t len);
ssize_t rb_read_fd(rb_buffer_t *b, int fd);
int rb_find_crlf(rb_buffer_t *b);
void rb_send(rb_channel_t *chl, const char *, size_t);
void rb_buffer_clear(rb_buffer_t *b);
void rb_buffer_destroy(rb_buffer_t **_b);

#endif /* _RIBEV_BUFFER_H */
