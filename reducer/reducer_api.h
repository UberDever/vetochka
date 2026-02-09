#ifndef __REDUCER_REDUCER_API_H__
#define __REDUCER_REDUCER_API_H__

#include "typedefs.h"
#include <stddef.h>

struct cells_t;
struct reducer_t;

#define REDUCER_DONE 1

#define REDUCER_APPLY_TOKEN SIZE_MAX

/**
 * Alloc and initialize a reducer_t structure with the given cells.
 * Reducer doesn't take ownership of the cells.
 * @return ERROR_SUCCESS on success, or error code on failure.
 */
error_t reducer_create(struct reducer_t** reducer, struct cells_t* cells);

/**
 * Deinit and free the reducer structure.
 */
void reducer_free(struct reducer_t** reducer);

/**
 * Deinit and init again the reducer structure.
 * Doesn't free memory of any sort.
 */
void reducer_reset(struct reducer_t* reducer);

/**
 * Perform a single step of the reducer.
 * @return ERROR_SUCCESS on success, error code on failure
 * or REDUCER_DONE if reduction is no longer possible
 */
error_t reducer_step(struct reducer_t* reducer);

/**
 * Get the error message of the reducer.
 * @return error message if error is present, or NULL if no error.
 */
const char* reducer_get_error(struct reducer_t* reducer);

/*
 * Pushes the cell index to reduction stack.
 * If index is REDUCER_APPLY_TOKEN,
 * then it is assumed that this token is apply postfix operation.
 */
void reducer_push_to_stack(struct reducer_t* reducer, size_t index);

/*
 * Returns the result of term reduction.
 */
size_t reducer_get_result(struct reducer_t* reducer);

#endif // __REDUCER_REDUCER_API_H__
