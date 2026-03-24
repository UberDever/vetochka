#ifndef __REDUCER_BYTECODE_IMPL_H__
#define __REDUCER_BYTECODE_IMPL_H__

#include "bytecode_api.h"

struct bytecode_reading_result_t {
    size_t line, col;
};

struct bytecode_writing_result_t {
    u8 ok;
};

#endif // __REDUCER_BYTECODE_IMPL_H__
