#define STB_DS_IMPLEMENTATION
#include "vendor_stbds.h"
#include "vendor/stb_ds.h"

void vendor_stbds_arrfree(void** arr) {
  stbds_arrfree(*arr);
}

size_t vendor_stbds_arrlenu(void* arr) {
  return stbds_arrlenu(arr);
}
