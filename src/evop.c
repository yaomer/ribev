#include <stddef.h>
#include "evop.h"
#include "config.h"

#ifdef RB_HAVE_KQUEUE
extern const rb_evop_t kqops;
#endif
#ifdef RB_HAVE_EPOLL
extern const rb_evop_t epops;
#endif
#ifdef RB_HAVE_POLL
extern const rb_evop_t pollops;
#endif

const rb_evop_t *evops[] = {
#ifdef RB_HAVE_KQUEUE
    &kqops,
#endif
#ifdef RB_HAVE_EPOLL
    &epops,
#endif
#ifdef RB_HAVE_POLL
    &pollops,
#endif
    NULL,
};
