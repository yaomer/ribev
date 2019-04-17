#include <stdio.h>
#include <unistd.h>
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
    int err;
    ssize_t n = rb_read_fd(chl->input, chl->ev.ident, &err);

    rb_log_debug("reads %zd bytes from fd=%d", n, chl->ev.ident);
    if (n > 0) {
        if (chl->unpackcb)
            chl->unpackcb(chl);
    } else if (n == 0) {
        rb_handle_close(chl);
    } else {
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
            rb_buffer_update_readidx(chl->output, n);
            if (rb_buffer_readable(chl->output) == 0)
                rb_chl_disable_write(chl);
        } else
            rb_log_error("write");
    }
}

void
rb_handle_event(rb_channel_t *chl)
{
    if (chl->ev.revents & RB_EV_ERROR) {
        rb_handle_error(chl);
    }
    if (chl->ev.revents & RB_EV_READ) {
        if (chl->readcb)
            chl->readcb(chl);
    }
    if (chl->ev.revents & RB_EV_WRITE) {
        if (chl->writecb)
            chl->writecb(chl);
    }
}
