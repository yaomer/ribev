#ifndef _RIBEV_EVLL_H
#define _RIBEV_EVLL_H

#include "fwd.h"

typedef struct rb_evll {
    rb_evthr_t **thrloop;
    int numloops;
    int nextloop;
} rb_evll_t;

rb_evll_t *rb_evll_init(int loops);
void rb_evll_run(rb_evll_t *evll);
rb_evthr_t *rb_evll_get_nextloop(rb_evll_t *evll);

#endif /* _RIBEV_EVLL_H */
