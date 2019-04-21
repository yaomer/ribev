#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "log.h"

static atomic_size_t alloc_cnt;

void *
rb_malloc(size_t nbytes)
{
    void *p;

    if ((p = malloc(nbytes)) == NULL)
        rb_log_error("malloc");
    alloc_cnt++;

    return p;
}

void *
rb_calloc(size_t n, size_t len)
{
    void *p;

    if ((p = calloc(n, len)) == NULL)
        rb_log_error("calloc");
    alloc_cnt++;

    return p;
}

void
rb_free(void *p)
{
    free(p);
    alloc_cnt--;
}

void
rb_checkmem(void)
{
    if (alloc_cnt)
        rb_log_warn("memory leak");
}
