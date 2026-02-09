#ifndef __REDUCER_REDUCER_IMPL_H__
#define __REDUCER_REDUCER_IMPL_H__

#include <stddef.h>
#include "reducer_api.h"

// NOTE: tests can know this structure to properly "mock" things
// basically, all things here are gettable, but don't change the pointers themselves!
typedef struct reducer_t {
    struct cells_t* cells;
    size_t* stack;
    size_t result;

    // private:
    size_t* stash;
    char* error;
} reducer_t;

#endif // __REDUCER_REDUCER_IMPL_H__
