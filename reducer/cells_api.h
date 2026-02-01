#ifndef __REDUCER_CELLS_API_H__
#define __REDUCER_CELLS_API_H__

#include "typedefs.h"

struct cells_t;

/**
 * Initialize a cells_t structure with the given capacity.
 * @return 0 on success, or -1 on failure.
 */
i64 cells_init(struct cells_t* cells, u64 capacity);

/**
 * Get the value at the specified index in the cells structure.
 * @return REDUCER_NODE_TYPE on success, or -1 on failure.
 */
i64 cells_get_node(struct cells_t* cells, u64 index, byte* out_value);

void cells_free(struct cells_t* cells);

#endif // __REDUCER_CELLS_API_H__
