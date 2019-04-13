#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

static atomic_size_t alloc_cnt;

void *
rb_malloc(size_t nbytes)
{
    void *p;

    if ((p = malloc(nbytes)) == NULL)
        ;
    atomic_fetch_add(&alloc_cnt, 1);

    return p;
}

void *
rb_calloc(size_t n, size_t len)
{
    void *p;

    if ((p = calloc(n, len)) == NULL)
        ;
    atomic_fetch_add(&alloc_cnt, 1);

    return p;
}

void
rb_free(void *p)
{
    atomic_fetch_sub(&alloc_cnt, 1);
    free(p);
}

void
rb_checkmem(void)
{
    if (alloc_cnt)
        ;
}
