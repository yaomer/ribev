#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include "event.h"
#include "buffer.h"
#include "channel.h"
#include "hash.h"
#include "evloop.h"
#include "log.h"

const char *
rb_eventstr(int events)
{
    if (events == RB_EV_READ)
        return "EV_READ";
    else if (events == RB_EV_WRITE)
        return "EV_WRITE";
    else if (events == RB_EV_ERROR)
        return "EV_ERROR";
    else
        return "none";
}

void
rb_handle_error(rb_channel_t *chl)
{
    socklen_t err;
    socklen_t len = sizeof(err);
    if (getsockopt(chl->ev.ident, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        rb_log_warn("fd=%d, %d", chl->ev.ident, err);
}

void
rb_handle_close(rb_channel_t *chl)
{
    rb_chl_del(chl);
}

void
rb_handle_read(rb_channel_t *chl)
{
    ssize_t n = rb_read_fd(chl->input, chl->ev.ident);

    rb_log_debug("reads %zd bytes from fd=%d", n, chl->ev.ident);
    if (n > 0) {
        if (chl->msgcb)
            chl->msgcb(chl);
    } else if (n == 0) {
        if (chl->closecb)
            chl->closecb(chl);
    } else {
        if (errno != EAGAIN
         && errno != EWOULDBLOCK
         && errno != EINTR)
            rb_handle_error(chl);
    }
}

void
rb_handle_write(rb_channel_t *chl)
{
    char *buf = rb_buffer_begin(chl->output);
    size_t readable = rb_buffer_readable(chl->output);

    if (rb_chl_is_writing(chl)) {
        ssize_t n = write(chl->ev.ident, buf, readable);
        rb_log_debug("writes %zd bytes to fd=%d", n, chl->ev.ident);
        if (n >= 0) {
            rb_buffer_retrieve(chl->output, n);
            /* 写完后，立即停止关注RB_EV_WRITE事件，防止busy loop */
            if (rb_buffer_readable(chl->output) == 0) {
                rb_chl_disable_write(chl);
                rb_chl_clear_flag(chl, RB_SENDING);
_close:
                if (chl->write_complete_cb)
                    chl->write_complete_cb(chl);
            }
        } else {
            /* 对端已关闭连接，此时只能丢掉未发完的数据，或许可以使用shutdown() */
            if (errno == EPIPE)
                goto _close;
            if (errno != EAGAIN
             && errno != EWOULDBLOCK
             && errno != EINTR)
                rb_log_error("write");
        }
    }
}

void
rb_handle_event(rb_channel_t *chl)
{
    if (chl->ev.revents & RB_EV_ERROR) {
        rb_handle_error(chl);
    }
    if (chl->ev.revents & RB_EV_WRITE) {
        if (chl->writecb)
            chl->writecb(chl);
    }
    /* readcb()可能会销毁chl，所以应该放在最后 */
    if (chl->ev.revents & RB_EV_READ) {
        if (chl->readcb)
            chl->readcb(chl);
    }
}
