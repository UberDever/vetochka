#include "domain_api.h"

#include "allocator_api.h"
#include "vendor/da.h"

error_t domain_da_byte_init(struct da_byte_t* array, const struct allocator_t* allocator) {
  if (array == NULL || allocator == NULL || allocator->vtable == NULL
      || allocator->vtable->remap == NULL || allocator->vtable->free == NULL) {
    return ERROR_INVALID_PARAM;
  }
  *array = CTOR(struct da_byte_t, .allocator = *allocator);
  return ERROR_SUCCESS;
}

void domain_da_byte_free(struct da_byte_t* array) {
  if (array == NULL) { return; }
  da_free(array);
}

span_cbyte_t domain_da_byte_get_span(const struct da_byte_t* array) {
  if (array == NULL) { return CTOR(span_cbyte_t, 0); }
  return CTOR(span_cbyte_t, .data = array->items, .len = array->count);
}
