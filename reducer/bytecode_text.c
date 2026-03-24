#include "bytecode_impl.h"

error_t bytecode_text_read(
    span_cbyte_t src,
    struct cells_t* cells,
    size_t* applications_stack,
    struct bytecode_reading_result_t* result) {
  (void)src;
  (void)cells;
  (void)applications_stack;
  (void)result;
  return ERROR_SUCCESS;
}

error_t bytecode_text_write(struct cells_t* cells, size_t* applications_stack, span_byte_t dst) {
  (void)cells;
  (void)applications_stack;
  (void)dst;
  return ERROR_SUCCESS;
}
