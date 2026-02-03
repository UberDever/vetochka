#ifndef __REDUCER_CELLS_DEBUG_H__
#define __REDUCER_CELLS_DEBUG_H__

#include "cells_api.h"

typedef void (*cells_print_fn)(void *ctx, const char *fmt, ...);

/**
 * Print a hex-view like visualization of the cells structure using the provided
 * print function. Left column: Hex dump of bytes, colored by node type. Right
 * column: Human-readable representation of nodes.
 */
void cells_print_debug_view(struct cells_t *cells, cells_print_fn print,
                            void *ctx);

#endif // __REDUCER_CELLS_DEBUG_H__
