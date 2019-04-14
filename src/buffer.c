#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include "buffer.h"
#include "vector.h"
#include "hash.h"
#include "alloc.h"

/*
 * [buffer]初始化时的大小，由于我们在rb_read_fd()中巧妙
 * 利用了[stack]空间，所以这个值不必太大，合适即可。
 */
#define RB_BUFFER_INIT_SIZE 1024

rb_buffer_t *
rb_buffer_init(void)
{
    rb_buffer_t *b = rb_malloc(sizeof(rb_buffer_t));

    b->buf = rb_vector_init(sizeof(char));
    b->readindex = b->writeindex = 0;
    rb_vector_reserve(b->buf, RB_BUFFER_INIT_SIZE);

    return b;
}

/*
 * 更新[b->readindex]字段。如果没读完，则将[readindex]后移；否则，
 * 就将[readindex]和[writeindex]置为初始位置。
 */
void
rb_buffer_update_readidx(rb_buffer_t *b, size_t len)
{
    if (len < rb_buffer_readable(b))
        b->readindex += len;
    else
        b->readindex = b->writeindex = 0;
}

/*
 * 如果某次写入的时候[buffer]中没有足够的空间，而前面经过多次读写留下了
 * 一个很大的[prependable]空间，此时我们就需要进行内部腾挪，将后面的数据
 * 挪到前面来，然后再写入。
 * 因为就算我们不腾挪，直接写入，还是会进行内存分配，依旧要将原数据拷贝到
 * 新的内存地址中，代价只会更大。
 */
void
rb_buffer_move_forward(rb_buffer_t *b, size_t len)
{
    size_t readable = rb_buffer_readable(b);
    size_t writeable = rb_buffer_writeable(b);
    size_t prependable = rb_buffer_prependable(b);

    if (len > writeable && len < prependable + writeable) {
        memcpy(rb_vector_entry(b->buf, 0), rb_buffer_begin(b), readable);
        b->readindex = 0;
        b->writeindex = readable;
    }
}

/*
 * 从[buffer]中读出[n - 1]个字符，并在末尾添加['\0']
 */
void
rb_buffer_read(rb_buffer_t *b, char *s, size_t n)
{
    size_t readable = rb_buffer_readable(b);

    if (n - 1 > readable) {
        n = readable + 1;
    }
    memcpy(s, rb_buffer_begin(b), n - 1);
    rb_buffer_update_readidx(b, n - 1);
    s[n] = '\0';
}

/*
 * 向[buffer]中写入[s]中前[len]个字符
 */
void
rb_buffer_write(rb_buffer_t *b, const char *s, size_t len)
{
    rb_vector_push_str(b->buf, s, len);
    b->writeindex += len;
}

/*
 * 将fd中的数据读入到[buffer]中，这里我们采用readv()分散读
 * 的方法，并结合栈空间[extrabuf]来避免一开始为每个[buffer]
 * 在堆上分配很大的内存。
 */
ssize_t
rb_read_fd(rb_buffer_t *b, int fd, int *perr)
{
    char extrabuf[65536];
    struct iovec iov[2];
    size_t writable = rb_buffer_writeable(b);
    ssize_t n;

    iov[0].iov_base = rb_vector_entry(b->buf, 0) + b->writeindex;
    iov[0].iov_len = writable;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);

    if ((n = readv(fd, iov, 2)) < 0)
        *perr = errno;
    else if (n <= writable)
        b->writeindex += n;
    else {  /* 将[extrabuf]中的数据追加到[buffer]中 */
        rb_vector_resize(b->buf, rb_vector_max_size(b->buf));
        b->writeindex += writable;
        rb_buffer_write(b, extrabuf, n - writable);
    }

    return n;
}

void
rb_buffer_clear(rb_buffer_t *b)
{
    rb_vector_resize(b->buf, 0);
    b->readindex = b->writeindex = 0;
}

void
rb_buffer_destroy(rb_buffer_t **_b)
{
    rb_buffer_t *b = *_b;

    rb_vector_destroy(&b->buf);
    rb_free((*_b));
    *_b = NULL;
}
