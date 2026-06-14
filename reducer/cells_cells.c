#include "cells_api.h"
#include "cells_impl.h"
#include "domain_api.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

error_t cells_create(struct cells_t** cells, size_t capacity) {
  if (!cells) { return ERROR_INVALID_PARAM; }
  *cells = calloc(1, sizeof(struct cells_t));
  if (!(*cells)) { return ERROR_NOMEM; }
  (*cells)->data = calloc(1, capacity);
  if (!(*cells)->data) {
    free(*cells);
    *cells = NULL;
    return ERROR_NOMEM;
  }
  (*cells)->occupied_bitmap = _bitmap_alloc(capacity);
  if (!(*cells)->occupied_bitmap) {
    free((*cells)->data);
    free(*cells);
    *cells = NULL;
    return ERROR_NOMEM;
  }
  (*cells)->free_chunks_head = calloc(1, sizeof(struct cells_free_chunk_t));
  if (!(*cells)->free_chunks_head) {
    free((*cells)->occupied_bitmap);
    free((*cells)->data);
    free(*cells);
    *cells = NULL;
    return ERROR_NOMEM;
  }
  (*cells)->free_chunks_head->index = 0;
  (*cells)->free_chunks_head->size = capacity;
  (*cells)->free_chunks_head->next = NULL;
  (*cells)->capacity = capacity;
  return ERROR_SUCCESS;
}

void cells_destroy(struct cells_t** cells) {
  if (!cells || !(*cells)) { return; }
  struct cells_free_chunk_t* free_chunk = (*cells)->free_chunks_head;
  while (free_chunk) {
    struct cells_free_chunk_t* next = free_chunk->next;
    free(free_chunk);
    free_chunk = next;
  }
  free((*cells)->data);
  free((*cells)->occupied_bitmap);
  free(*cells);
  *cells = NULL;
}

span_cbyte_t cells_get_span(const struct cells_t* cells) {
  if (!cells) { return (span_cbyte_t){0}; }
  return (span_cbyte_t){.data = cells->data, .len = cells->capacity};
}

struct cells_node_encoding_t {
  cells_node_type_t type;
  enum cells_node_layout_t layout;
  size_t fixed_encoded_size;
  byte code;
};

static struct cells_node_encoding_t encoding_from_byte(byte value) {
#define MATCH_ENCODING(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                          \
  if ((value & MASK) == CODE) {                                                                    \
    return (struct cells_node_encoding_t){                                                         \
        .type = {.value = CELLS_NODE_TYPE_##TYPE},                                                 \
        .layout = CELLS_NODE_LAYOUT_##LAYOUT,                                                      \
        .fixed_encoded_size = SIZE,                                                                \
        .code = CODE,                                                                              \
    };                                                                                             \
  }

  CELLS_NODE_INFO_ITEMS(MATCH_ENCODING)

#undef MATCH_ENCODING

  return (struct cells_node_encoding_t){0};
}

static struct cells_node_encoding_t encoding_from_header(
    cells_node_type_t type, size_t encoded_size) {
#define MATCH_ENCODING(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                          \
  if (type.value == CELLS_NODE_TYPE_##TYPE && ((SIZE) == 0 || encoded_size == (SIZE))) {           \
    return (struct cells_node_encoding_t){                                                         \
        .type = type,                                                                              \
        .layout = CELLS_NODE_LAYOUT_##LAYOUT,                                                      \
        .fixed_encoded_size = SIZE,                                                                \
        .code = CODE,                                                                              \
    };                                                                                             \
  }

  CELLS_NODE_INFO_ITEMS(MATCH_ENCODING)

#undef MATCH_ENCODING

  return (struct cells_node_encoding_t){0};
}

static inline u16 read_u16_be(const byte* p) {
  return ((u16)p[0] << 8) | (u16)p[1];
}

static inline void write_u16_be(byte* p, u16 v) {
  p[0] = (byte)(v >> 8);
  p[1] = (byte)(v & 0xffU);
}

static inline u64 read_u64_be(const byte* p) {
  u64 v = 0;
  for (size_t i = 0; i < sizeof(v); i++) {
    v = (v << 8) | (u64)p[i];
  }
  return v;
}

static inline void write_u64_be(byte* p, u64 v) {
  for (size_t i = 0; i < sizeof(v); i++) {
    p[sizeof(v) - 1 - i] = (byte)(v & 0xffU);
    v >>= 8;
  }
}

static inline i64 read_i64_le(const byte* p) {
  u64 value = 0;
  for (size_t i = 0; i < sizeof(value); i++) {
    value |= (u64)p[i] << (i * 8);
  }
  return (i64)value;
}

static inline void write_i64_le(byte* p, i64 value) {
  u64 raw = (u64)value;
  for (size_t i = 0; i < sizeof(raw); i++) {
    p[i] = (byte)(raw >> (i * 8));
  }
}

struct cells_node_header_t cells_get_node_header(struct cells_t* cells, size_t index) {
  struct cells_node_header_t header = {0};
  if (!cells || index >= cells->capacity) { return header; }
  struct cells_node_encoding_t encoding = encoding_from_byte(cells->data[index]);
  if (encoding.layout == CELLS_NODE_LAYOUT_INVALID) { return header; }
  header.type = encoding.type;

  if (encoding.layout == CELLS_NODE_LAYOUT_BYTES) {
    if (cells->capacity - index < 2) { return (struct cells_node_header_t){0}; }
    size_t uleb_len = 0;
    size_t uleb_val = 0;
    if (uleb128_read(cells->data, cells->capacity, index + 1, &uleb_len, &uleb_val) != 0) {
      return (struct cells_node_header_t){0};
    }
    if (uleb_len > cells->capacity - index - 1
        || uleb_val > cells->capacity - index - 1 - uleb_len) {
      return (struct cells_node_header_t){0};
    }
    header.encoded_size = 1 + uleb_len + uleb_val;
  } else {
    header.encoded_size = encoding.fixed_encoded_size;
  }

  if (header.encoded_size == 0 || header.encoded_size > cells->capacity - index) {
    return (struct cells_node_header_t){0};
  }

  for (size_t i = index; i < index + header.encoded_size; i++) {
    if (!_bitmap_get_bit(cells->occupied_bitmap, i)) { return (struct cells_node_header_t){0}; }
  }

  return header;
}

struct cells_node_t cells_get_node(
    struct cells_t* cells, size_t index, struct cells_node_header_t header) {
  struct cells_node_t node = {0};
  if (!cells || index >= cells->capacity || header.encoded_size > cells->capacity - index) {
    return node;
  }

  struct cells_node_encoding_t encoding = encoding_from_header(header.type, header.encoded_size);
  switch (encoding.layout) {
    case CELLS_NODE_LAYOUT_REF14: {
      i16 offset = 0;
      if ((cells->data[index] & 0xC0) != encoding.code) { return node; }
      if (index + sizeof(offset) > cells->capacity) { return node; }
      u16 raw = read_u16_be(cells->data + index);
      offset = (i16)(raw & 0x3fffU);
      if ((offset & 0x2000) != 0) { offset |= (i16)0xc000; }
      node.header = header;
      node.as.ref = offset;
      return node;
    }

    case CELLS_NODE_LAYOUT_REF62: {
      i64 offset = 0;
      if ((cells->data[index] & 0xC0) != encoding.code) { return node; }
      if (index + sizeof(offset) > cells->capacity) { return node; }
      u64 raw = read_u64_be(cells->data + index);
      offset = (i64)(raw & UINT64_C(0x3fffffffffffffff));
      if ((offset & (i64)UINT64_C(0x2000000000000000)) != 0) {
        offset |= (i64)UINT64_C(0xc000000000000000);
      }
      node.header = header;
      node.as.ref = offset;
      return node;
    }
    case CELLS_NODE_LAYOUT_TAG:
      if (encoding.code != cells->data[index]) { return node; }
      node.header = header;
      return node;
    case CELLS_NODE_LAYOUT_I64: {
      if (encoding.code != cells->data[index]) { return node; }
      if (sizeof(i64) + 1 > cells->capacity - index) { return node; }
      node.header = header;
      node.as.nativef = read_i64_le(cells->data + index + 1);
      return node;
    }
    case CELLS_NODE_LAYOUT_BYTES: {
      if (encoding.code != cells->data[index]) { return node; }
      struct span_byte_t payload = {0};
      size_t uleb_len = 0;
      size_t uleb_val = 0;
      if (uleb128_read(cells->data, cells->capacity, index + 1, &uleb_len, &uleb_val) != 0) {
        return node;
      }
      if (uleb_len > cells->capacity - index - 1
          || uleb_val > cells->capacity - index - 1 - uleb_len) {
        return node;
      }
      payload.len = uleb_val;
      payload.data = cells->data + index + 1 + uleb_len;
      node.as.nativev = payload;
      node.header = header;
      return node;
    }
    default: return node;
  }
}

typedef struct free_chunk_pair_t {
  struct cells_free_chunk_t* prev;
  struct cells_free_chunk_t* chunk;
} free_chunk_pair_t;

error_t cells_alloc_chunk(struct cells_t* cells, size_t chunk_size, size_t* index_out) {
  if (!cells || !index_out) { return ERROR_INVALID_PARAM; }
  if (chunk_size == 0) {
    *index_out = 0;
    return ERROR_SUCCESS;
  }
  free_chunk_pair_t pair = {0};
  pair.chunk = cells->free_chunks_head;
  while (pair.chunk) {
    if (pair.chunk->size >= chunk_size) {
      *index_out = pair.chunk->index;
      pair.chunk->index += chunk_size;
      pair.chunk->size -= chunk_size;
      if (pair.chunk->size == 0) {
        if (pair.prev) {
          pair.prev->next = pair.chunk->next;
        } else {
          cells->free_chunks_head = pair.chunk->next;
        }
        free(pair.chunk);
      }
      for (size_t j = *index_out; j < *index_out + chunk_size; j++) {
        _bitmap_set_bit(cells->occupied_bitmap, j, 1);
      }
      return ERROR_SUCCESS;
    }
    pair.prev = pair.chunk;
    pair.chunk = pair.chunk->next;
  }

  return ERROR_NOMEM;
}

error_t cells_alloc_chunk_with_refs(
    struct cells_t* cells,
    size_t chunk_size,
    struct opt_size_t referenced_lhs,
    struct opt_size_t referenced_rhs,
    size_t* index_out) {
  if (!cells || !index_out) { return ERROR_INVALID_PARAM; }
  if (chunk_size == 0) {
    *index_out = 0;
    return ERROR_SUCCESS;
  }
  if (!referenced_lhs.has_value) { return ERROR_INVALID_PARAM; }
  // to try (with addition to chunk_size):
  // 2bytes(lhs2)
  // 4bytes(lhs2, rhs2)
  // 8bytes(lhs8)
  // 10bytes(lhs2, rhs8 or lhs8, rhs2)
  // 16bytes(lhs8, rhs8)
  const size_t configs_len = 6;
  struct pair_size_t_size_t configs[configs_len];
  configs[0] = (struct pair_size_t_size_t){sizeof(i16), 0};
  configs[1] = (struct pair_size_t_size_t){sizeof(i16), sizeof(i16)};
  configs[2] = (struct pair_size_t_size_t){sizeof(i64), 0};
  configs[3] = (struct pair_size_t_size_t){sizeof(i16), sizeof(i64)};
  configs[4] = (struct pair_size_t_size_t){sizeof(i64), sizeof(i16)};
  configs[5] = (struct pair_size_t_size_t){sizeof(i64), sizeof(i64)};
  free_chunk_pair_t* free_chunks = NULL;
  size_t selected_config = 0;
  size_t selected_chunk = 0;
  for (selected_config = 0; selected_config < configs_len; ++selected_config) {
    size_t need_size =
        chunk_size + configs[selected_config].first + configs[selected_config].second;
    free_chunk_pair_t pair = {0};
    pair.chunk = cells->free_chunks_head;
    while (pair.chunk) {
      if (pair.chunk->size >= need_size) { stbds_arrput(free_chunks, pair); }
      pair.prev = pair.chunk;
      pair.chunk = pair.chunk->next;
    }
    if (stbds_arrlenu(free_chunks) == 0) { continue; }
    bool found_config = false;
    for (selected_chunk = 0; selected_chunk < stbds_arrlenu(free_chunks); ++selected_chunk) {
      struct cells_free_chunk_t* free_chunk = free_chunks[selected_chunk].chunk;
      size_t supposed_index = free_chunk->index;
      i64 ref_lhs = (i64)referenced_lhs.value - ((i64)supposed_index + chunk_size);
      i64 ref_rhs = 0;
      if (referenced_rhs.has_value) {
        ref_rhs = (i64)referenced_rhs.value
                  - ((i64)supposed_index + chunk_size + configs[selected_config].first);
      }
      switch (selected_config) {
        case 0:
          if (cells_ref_fits_ref14(ref_lhs) && !referenced_rhs.has_value) {
            found_config = true;
            break;
          }
          continue;
        case 1:
          if (cells_ref_fits_ref14(ref_lhs) && cells_ref_fits_ref14(ref_rhs)) {
            found_config = true;
            break;
          }
          continue;
        case 2:
          if (cells_ref_fits_ref62(ref_lhs) && !referenced_rhs.has_value) {
            found_config = true;
            break;
          }
          continue;
        case 3:
          if (cells_ref_fits_ref14(ref_lhs) && cells_ref_fits_ref62(ref_rhs)) {
            found_config = true;
            break;
          }
          continue;
        case 4:
          if (cells_ref_fits_ref62(ref_lhs) && cells_ref_fits_ref14(ref_rhs)) {
            found_config = true;
            break;
          }
          continue;
        case 5:
          if (cells_ref_fits_ref62(ref_lhs) && cells_ref_fits_ref62(ref_rhs)) {
            found_config = true;
            break;
          }
          continue;
      }
      if (found_config) { break; }
    }
    if (found_config) { break; }
    stbds_arrsetlen(free_chunks, 0);
  }
  error_t err = ERROR_SUCCESS;
  // can't find free chunk that would be able to store biggest size for references and be
  // able to reference referenced nodes
  if (selected_config >= configs_len) {
    err = ERROR_NOMEM;
    goto defer;
  }
  if (selected_chunk >= stbds_arrlenu(free_chunks)) {
    err = ERROR_NOMEM;
    goto defer;
  }
  size_t need_size = chunk_size + configs[selected_config].first + configs[selected_config].second;
  free_chunk_pair_t pair = free_chunks[selected_chunk];
  *index_out = pair.chunk->index;
  pair.chunk->index += need_size;
  pair.chunk->size -= need_size;
  if (pair.chunk->size == 0) {
    if (pair.prev) {
      pair.prev->next = pair.chunk->next;
    } else {
      cells->free_chunks_head = pair.chunk->next;
    }
    free(pair.chunk);
  }
  for (size_t j = *index_out; j < *index_out + need_size; j++) {
    _bitmap_set_bit(cells->occupied_bitmap, j, 1);
  }
defer:
  stbds_arrfree(free_chunks);
  return err;
}

error_t cells_write_node(struct cells_t* cells, size_t index, struct cells_node_t node) {
  if (!cells || index >= cells->capacity) { return ERROR_INVALID_PARAM; }

  struct cells_node_encoding_t encoding =
      encoding_from_header(node.header.type, node.header.encoded_size);
  switch (encoding.layout) {
    case CELLS_NODE_LAYOUT_REF14: {
      u16 offset = 0;
      if (index + sizeof(offset) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      if (!cells_ref_fits_ref14(node.as.ref)) { return ERROR_INVALID_PARAM; }
      offset = (u16)((u16)encoding.code << 8) | (u16)(node.as.ref & 0x3fff);
      write_u16_be(cells->data + index, offset);
      return ERROR_SUCCESS;
    }
    case CELLS_NODE_LAYOUT_REF62: {
      u64 offset = 0;
      if (index + sizeof(offset) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      if (!cells_ref_fits_ref62(node.as.ref)) { return ERROR_INVALID_PARAM; }
      offset = (u64)encoding.code << 56 | (u64)(node.as.ref & INT64_C(0x3fffffffffffffff));
      write_u64_be(cells->data + index, offset);
      return ERROR_SUCCESS;
    }
    case CELLS_NODE_LAYOUT_TAG:
      if (index + 1 > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      cells->data[index] = encoding.code;
      return ERROR_SUCCESS;
    case CELLS_NODE_LAYOUT_I64:
      if (index + 1 + sizeof(i64) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      cells->data[index] = encoding.code;
      write_i64_le(cells->data + index + 1, node.as.nativef);
      return ERROR_SUCCESS;
    case CELLS_NODE_LAYOUT_BYTES: {
      if (index + 1 > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      size_t uleb_len = uleb128_size(node.as.nativev.len);
      if (node.header.encoded_size != 1 + uleb_len + node.as.nativev.len) {
        return ERROR_INVALID_PARAM;
      }
      if (uleb_len > cells->capacity - index - 1
          || node.as.nativev.len > cells->capacity - index - 1 - uleb_len) {
        return ERROR_OUT_OF_BOUNDS;
      }
      cells->data[index] = encoding.code;
      uleb128_write(cells->data + index + 1, node.as.nativev.len);
      memcpy(cells->data + index + 1 + uleb_len, node.as.nativev.data, node.as.nativev.len);
      return ERROR_SUCCESS;
    }
    default: return ERROR_INVALID_PARAM;
  }
}

error_t cells_node_free(struct cells_t* cells, size_t index, size_t node_size) {
  if (!cells) { return ERROR_INVALID_PARAM; }
  if (node_size == 0) { return ERROR_SUCCESS; }
  if (index >= cells->capacity || node_size > cells->capacity - index) {
    return ERROR_OUT_OF_BOUNDS;
  }

  size_t end = index + node_size;
  struct cells_free_chunk_t* prev = NULL;
  struct cells_free_chunk_t* free_chunk = cells->free_chunks_head;
  while (free_chunk && free_chunk->index < index) {
    prev = free_chunk;
    free_chunk = free_chunk->next;
  }

  if (prev) {
    size_t prev_end = prev->index + prev->size;
    if (prev_end > index) { return ERROR_INVALID_PARAM; }
  }
  if (free_chunk && end > free_chunk->index) { return ERROR_INVALID_PARAM; }

  bool merge_prev = prev && (prev->index + prev->size == index);
  bool merge_next = free_chunk && (end == free_chunk->index);
  if (merge_prev && merge_next) {
    prev->size += node_size + free_chunk->size;
    prev->next = free_chunk->next;
    free(free_chunk);
  } else if (merge_prev) {
    prev->size += node_size;
  } else if (merge_next) {
    free_chunk->index = index;
    free_chunk->size += node_size;
  } else {
    struct cells_free_chunk_t* new_chunk = calloc(1, sizeof(struct cells_free_chunk_t));
    if (!new_chunk) { return ERROR_NOMEM; }
    new_chunk->index = index;
    new_chunk->size = node_size;
    new_chunk->next = free_chunk;
    if (prev) {
      prev->next = new_chunk;
    } else {
      cells->free_chunks_head = new_chunk;
    }
  }

  memset(cells->data + index, 0, node_size);
  for (size_t i = index; i < end; ++i) {
    _bitmap_set_bit(cells->occupied_bitmap, i, 0);
  }
  return ERROR_SUCCESS;
}
