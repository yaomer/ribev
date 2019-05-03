/*
 * [buffer]借用[vector]来进行内存管理，所以实现很简单
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include "channel.h"
#include "buffer.h"
#include "vector.h"
#include "hash.h"
#include "evloop.h"
#include "alloc.h"
#include "task.h"
#include "log.h"

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
    b->readindex = b->writeindex = RB_BUFFER_PREPEND;
    rb_vector_reserve(b->buf, RB_BUFFER_INIT_SIZE);
    rb_vector_resize(b->buf, RB_BUFFER_PREPEND);

    return b;
}

/*
 * 更新[b->readindex]字段。如果没读完，则将[readindex]后移；否则，
 * 就将[readindex]和[writeindex]置为初始位置。
 */
void
rb_buffer_retrieve(rb_buffer_t *b, size_t len)
{
    if (len < rb_buffer_readable(b))
        b->readindex += len;
    else
        b->readindex = b->writeindex = RB_BUFFER_PREPEND;
}

/*
 * 如果某次写入的时候[buffer]中没有足够的空间，而前面经过多次读写留下了
 * 一个很大的[prependable]空间，此时我们就需要进行内部腾挪，将后面的数据
 * 挪到前面来，然后再写入。
 * 因为就算我们不腾挪，直接写入，还是会进行内存分配，依旧要将原数据拷贝到
 * 新的内存地址中，代价只会更大。
 */
void
rb_buffer_make_space(rb_buffer_t *b, size_t len)
{
    size_t readable = rb_buffer_readable(b);
    size_t writeable = rb_buffer_writeable(b);
    size_t prependable = rb_buffer_prependable(b);

    if (len > writeable && len < prependable + readable) {
        memcpy(rb_vector_entry(b->buf, 0), rb_buffer_begin(b), readable);
        b->readindex = RB_BUFFER_PREPEND;
        b->writeindex = b->readindex + readable;
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
    rb_buffer_retrieve(b, n - 1);
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
rb_read_fd(rb_buffer_t *b, int fd)
{
    char extrabuf[65536];
    struct iovec iov[2];
    size_t writable = rb_buffer_writeable(b);
    ssize_t n;

    iov[0].iov_base = rb_vector_entry(b->buf, 0) + b->writeindex;
    iov[0].iov_len = writable;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);

    if ((n = readv(fd, iov, 2)) > 0) {
        if (n <= writable)
            b->writeindex += n;
        else {  /* 将[extrabuf]中的数据追加到[buffer]中 */
            rb_vector_resize(b->buf, rb_vector_max_size(b->buf));
            b->writeindex += writable;
            rb_buffer_write(b, extrabuf, n - writable);
        }
    }

    return n;
}

/*
 * 返回[\r\n]在[buffer]中第一次出现的位置。
 */
int
rb_find_crlf(rb_buffer_t *b)
{
    char *s = rb_buffer_begin(b);
    char *p = strnstr(s, "\r\n", rb_buffer_readable(b));
    return p ? p - s : -1;
}

static void
rb_send_in_loop(rb_channel_t *chl, const char *s, size_t len)
{
    if (!rb_chl_is_writing(chl) && rb_buffer_readable(chl->output) == 0) {
        ssize_t n = write(chl->ev.ident, s, len);
        if (n > 0) {
            if (n < len) {
                /* 没写完，将剩余的数据添加到[output buffer]中，并注册RB_EV_WRITE事件，
                 * 当fd变得可写时，再继续发送 */
                rb_buffer_write(chl->output, s + n, len - n);
                rb_buffer_retrieve(chl->output, len - n);
                rb_chl_enable_write(chl);
            } else {
                rb_chl_clear_flag(chl, RB_SENDING);
                if (chl->write_complete_cb)
                    chl->write_complete_cb(chl);
            }
        } else {
            if (errno != EAGAIN
             && errno != EWOULDBLOCK
             && errno != EINTR)
                rb_log_error("write");
        }
    } else {
        /* 如果有新到来的消息时，[output buffer]中还有未发完的数据，就将
         * 新到来的消息追加到它的末尾，之后统一发送。(这样才能保证接收方
         * 接收到消息的正确性) */
        rb_buffer_make_space(chl->output, len);
        rb_buffer_write(chl->output, s, len);
        rb_buffer_retrieve(chl->output, len);
    }
}

static void
__rb_send_in_loop(void **argv)
{
    rb_channel_t *chl = (rb_channel_t *)argv[0];
    char *s = (char *)argv[1];
    size_t len = (size_t)argv[2];
    rb_send_in_loop(chl, s, len);
}

/*
 * rb_send是线程安全的。
 */
void
rb_send(rb_channel_t *chl, const char *s, size_t len)
{
    rb_chl_set_flag(chl, RB_SENDING);
    if (rb_in_loop_thread(chl->loop))
        rb_send_in_loop(chl, s, len);
    else {
        void *ps = rb_malloc(len);
        memcpy(ps, s, len);
        rb_task_t *t = rb_alloc_task(3);
        t->callback = __rb_send_in_loop;
        t->argv[0] = chl;
        t->argv[1] = ps;
        t->argv[2] = (void *)len;
        t->free_argv[1] = rb_free;
        rb_run_in_loop(chl->loop, t);
    }
}

/*
 * 清空缓冲区
 */
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
