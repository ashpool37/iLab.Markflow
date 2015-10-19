#ifndef _PRECOND_H_
#define _PRECOND_H_
#include <errno.h>

#define _PRECONDITION(eval, seterr, action)                          \
    if (!(eval))                                                    \
    {                                                               \
        errno = seterr;                                             \
        action;                                                     \
    }                                                               \

#endif
