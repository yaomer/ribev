#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "event.h"
#include "buffer.h"
#include "channel.h"
#include "hash.h"
#include "evloop.h"

void
rb_handle_error(rb_channel_t *chl)
{
    socklen_t err;
    socklen_t len = sizeof(err);
    if (getsockopt(chl->ev.ident, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        ;
}

void
rb_handle_close(rb_channel_t *chl)
{
    rb_hash_delete(chl->loop->chlist, chl->ev.ident);
    close(chl->ev.ident);
}

void
rb_handle_read(rb_channel_t *chl)
{
    int err;
    ssize_t n = rb_read_fd(chl->input, chl->ev.ident, &err);

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
        if (n > 0) {
            rb_buffer_update_readidx(chl->output, n);
            if (rb_buffer_readable(chl->output) == 0)
                rb_chl_disable_write(chl);
        } else
            ;
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
