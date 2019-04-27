#include "alloc.h"
#include "evthr.h"
#include "evll.h"

/*
 * 一个[evthr]池子，正合多线程之义。
 */
rb_evll_t *
rb_evll_init(int loops)
{
    rb_evll_t *evll = rb_malloc(sizeof(rb_evll_t));

    evll->numloops = loops;
    evll->thrloop = rb_calloc(loops, sizeof(rb_evthr_t *));
    for (int i = 0; i < loops; i++)
        evll->thrloop[i] = rb_evthr_init();
    evll->nextloop = 0;

    return evll;
}

void
rb_evll_run(rb_evll_t *evll)
{
    for (int i = 0; i < evll->numloops; i++)
        rb_evthr_run(evll->thrloop[i]);
}

/*
 * 这里采用[round-robin]的方法来挑选下一个用来添加connfd的loop，
 * 虽然简单却能保证基本的负载均衡。
 */
rb_evthr_t *
rb_evll_get_nextloop(rb_evll_t *evll)
{
    if (evll->nextloop >= evll->numloops)
        evll->nextloop = 0;
    return evll->thrloop[evll->nextloop++];
}

void
rb_evll_quit(rb_evll_t *evll)
{
    for (int i = 0; i < evll->numloops; i++)
        rb_evthr_quit(evll->thrloop[i]);
}
