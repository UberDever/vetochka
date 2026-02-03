#include "cells_api.h"
#include "cells_impl.h"
#include "typedefs.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SIGNED_VALUES_META()                                                                       \
  switch (meta.type) {                                                                             \
    case CELLS_NODE_TYPE_REF1:                                                                     \
      tag = 0x00;                                                                                  \
      total_bits = 5;                                                                              \
      break;                                                                                       \
    case CELLS_NODE_TYPE_REF2:                                                                     \
      tag = 0x20;                                                                                  \
      total_bits = 13;                                                                             \
      break;                                                                                       \
    case CELLS_NODE_TYPE_REF4:                                                                     \
      tag = 0x40;                                                                                  \
      total_bits = 29;                                                                             \
      break;                                                                                       \
    case CELLS_NODE_TYPE_REF8:                                                                     \
      tag = 0x60;                                                                                  \
      total_bits = 61;                                                                             \
      break;                                                                                       \
    case CELLS_NODE_TYPE_NATIVE0F:                                                                 \
      tag = 0x00;                                                                                  \
      total_bits = 53;                                                                             \
      break;                                                                                       \
    case CELLS_NODE_TYPE_NATIVE1F:                                                                 \
      tag = 0x20;                                                                                  \
      total_bits = 53;                                                                             \
      break;                                                                                       \
    case CELLS_NODE_TYPE_NATIVE2F:                                                                 \
      tag = 0x40;                                                                                  \
      total_bits = 53;                                                                             \
      break;                                                                                       \
    default: fprintf(stderr, "%d\n", meta.type); assert(0 && "unreachable");                       \
  }

static int read_signed_int(
    const byte* base, size_t cap, size_t index, struct cells_node_meta_t meta, int64_t* out_value) {
  if (!base || !out_value) { return -1; }
  uint8_t tag;
  unsigned total_bits;
  SIGNED_VALUES_META();

  size_t bytes = (total_bits <= 5) ? 1 : 1 + ((total_bits - 5 + 7) / 8);
  if (index + bytes > cap) { return -1; }

  const byte* cur = base + index;
  if ((*cur & 0xE0U) != tag) { return -3; }

  uint64_t raw = *cur & 0x1FU;
  cur++;
  for (size_t i = 1; i < bytes; i++) {
    raw |= (uint64_t)(*cur++) << (5 + 8 * (i - 1));
  }

  // Sign extend
  int64_t val = (int64_t)(raw << (64 - total_bits));
  val >>= (64 - total_bits);

  *out_value = val;
  return 0;
}

static int write_signed_int(
    byte* base, size_t cap, size_t index, struct cells_node_meta_t meta, int64_t value) {
  if (!base) { return -1; }
  uint8_t tag;
  unsigned total_bits;
  SIGNED_VALUES_META();

  size_t bytes = (total_bits <= 5) ? 1 : 1 + ((total_bits - 5 + 7) / 8);
  if (index + bytes > cap) { return -1; }

  // Check if value fits in total_bits
  int64_t check = (int64_t)((uint64_t)value << (64 - total_bits)) >> (64 - total_bits);
  if (check != value) { return -2; }

  byte* cur = base + index;
  uint64_t u = (uint64_t)value;

  *cur++ = (byte)(tag | (u & 0x1FU));
  u >>= 5;

  for (size_t i = 1; i < bytes; i++) {
    *cur++ = (byte)u;
    u >>= 8;
  }
  return 0;
}

#undef SIGNED_VALUES_META

static inline int uleb128_read(
    const byte* data, size_t capacity, size_t index, size_t* uleb_len, size_t* uleb_value) {
  size_t shift = 0;
  while (1) {
    if (index > capacity) { return -1; }
    byte b = data[index];
    size_t chunk = b & 0x7f;

    // overflow / shift guard
    if (shift > 8 * sizeof(size_t)) { return -2; }
    // also guard that shifting chunk won't overflow (conservative)
    if (chunk != 0 && shift > 8 * sizeof(size_t) - 7) { return -2; }

    *uleb_value |= chunk << shift;
    shift += 7;
    *uleb_len += 1;
    index++;

    if ((b & 0x80) == 0) { break; }
    if (*uleb_len > 9) { assert(0 && "uleb_len > 9"); }
  }
  return 0;
}

static size_t uleb128_size(u64 x) {
  size_t n = 1;
  while (x >= 0x80) {
    x >>= 7;
    n++;
  }
  return n;
}

// writes ULEB128 for x into out, returns bytes written (>=1)
static size_t uleb128_write(byte* out, u64 x) {
  size_t n = 0;
  do {
    byte b = (byte)(x & 0x7F);
    x >>= 7;
    if (x) { b |= 0x80; }
    out[n++] = b;
  } while (x);
  return n;
}

static inline int read_payload(
    const byte* data,
    size_t capacity,
    size_t index,
    struct cells_node_meta_t meta,
    struct span_byte_t* out_data) {
  if (!data || !out_data) { return -1; }
  if (index >= capacity) { return -1; }

  byte b0 = data[index];
  byte tag = (byte)(b0 >> 5);
  size_t lo5 = (size_t)(b0 & 0x1FU);
  switch (tag) {
    case 0x04:
      if (meta.type != CELLS_NODE_TYPE_NATIVE0V) { return -1; };
      break;
    case 0x05:
      if (meta.type != CELLS_NODE_TYPE_NATIVE1V) { return -1; };
      ;
      break;
    case 0x06:
      if (meta.type != CELLS_NODE_TYPE_NATIVE2V) { return -1; };
      break;
    default: return -2;
  }
  size_t uleb_len = 0;
  size_t uleb_value = 0;
  if (index + 1 > capacity) { return -1; }
  int rc = uleb128_read(data, capacity, index + 1, &uleb_len, &uleb_value);
  if (rc != 0) { return rc; }
  // Guard against overflow of size_t
  if (uleb_value > (SIZE_MAX >> 5)) { return -3; }
  size_t len = lo5 | (uleb_value << 5);
  size_t header_total = 1 + uleb_len;
  if (index + header_total > capacity) { return -1; }
  if (len > capacity - (index + header_total)) { return -1; }
  out_data->len = len;
  out_data->data = (byte*)(data + index + header_total);
  return 0;
}

static int write_payload(byte* data, size_t capacity, size_t index, struct cells_node_t node) {
  byte tag = 0;
  if (node.meta.type == CELLS_NODE_TYPE_NATIVE0V) {
    tag = 0x04;
  } else if (node.meta.type == CELLS_NODE_TYPE_NATIVE1V) {
    tag = 0x05;
  } else if (node.meta.type == CELLS_NODE_TYPE_NATIVE2V) {
    tag = 0x06;
  }

  size_t total = 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  if (index + total > capacity) { return -1; }
  data[index] = (byte)((tag << 5) | (byte)(node.as.nativev.len & 0x1fU));
  size_t w = uleb128_write(data + index + 1, node.as.nativev.len >> 5);
  assert(node.as.nativev.data && "node.as.nativev.data");
  memcpy(data + index + 1 + w, node.as.nativev.data, node.as.nativev.len);
  return 0;
}

int cells_init(struct cells_t** cells, size_t capacity) {
  *cells = calloc(1, sizeof(struct cells_t));
  (*cells)->data = calloc(1, capacity);
  if (!(*cells)->data) { return -1; }
  (*cells)->occupied_bitmap = _bitmap_alloc(capacity);
  if (!(*cells)->occupied_bitmap) {
    free((*cells)->data);
    return -1;
  }
  (*cells)->capacity = capacity;
  return 0;
}

void cells_free(struct cells_t** cells) {
  free((*cells)->data);
  free((*cells)->occupied_bitmap);
  free(*cells);
}

struct cells_node_meta_t cells_get_node_meta(struct cells_t* cells, size_t index) {
  struct cells_node_meta_t meta = {0};
  if (index > cells->capacity) { return meta; }
  byte b = cells->data[index];

  if ((b & 0xe0) == 0) {
    meta.type = CELLS_NODE_TYPE_REF1;
    meta.size = 1;
    goto check_occupied;
  }
  if ((b & 0xe0) >> 5 == 1) {
    meta.type = CELLS_NODE_TYPE_REF2;
    meta.size = 2;
  }
  if ((b & 0xe0) >> 5 == 2) {
    meta.type = CELLS_NODE_TYPE_REF4;
    meta.size = 4;
    goto check_occupied;
  }
  if ((b & 0xe0) >> 5 == 3) {
    meta.type = CELLS_NODE_TYPE_REF8;
    meta.size = 8;
    goto check_occupied;
  }

  if (b == 0xc0) {
    meta.type = CELLS_NODE_TYPE_TREE0;
    meta.size = 1;
    goto check_occupied;
  }
  if (b == 0xe0) {
    meta.type = CELLS_NODE_TYPE_TREE1;
    meta.size = 1;
    goto check_occupied;
  }
  if (b == 0xd0) {
    meta.type = CELLS_NODE_TYPE_TREE2;
    meta.size = 1;
    goto check_occupied;
  }

  // native payload
  if (b == 0x80) {
    if (index + 1 > cells->capacity) { return meta; }
    byte b2 = cells->data[index + 1];
    if ((b2 & 0xe0) >> 5 == 0) {
      meta.type = CELLS_NODE_TYPE_NATIVE0F;
      meta.size = 1 + sizeof(int64_t);
      goto check_occupied;
    }
    if ((b2 & 0xe0) >> 5 == 1) {
      meta.type = CELLS_NODE_TYPE_NATIVE1F;
      meta.size = 1 + sizeof(int64_t);
      goto check_occupied;
    }
    if ((b2 & 0xe0) >> 5 == 2) {
      meta.type = CELLS_NODE_TYPE_NATIVE2F;
      meta.size = 1 + sizeof(int64_t);
      goto check_occupied;
    }

    byte tag = (b2 & 0xe0) >> 5;
    size_t lo5 = b2 & 0x1f;
    size_t uleb_len = 0;
    size_t uleb_val = 0;
    if (uleb128_read(cells->data, cells->capacity, index + 2, &uleb_len, &uleb_val) != 0) {
      return meta;
    }
    size_t len = lo5 | (uleb_val << 5);
    meta.size = 1 + 1 + uleb_len + len;

    if (tag == 0x04) { meta.type = CELLS_NODE_TYPE_NATIVE0V; }
    if (tag == 0x05) { meta.type = CELLS_NODE_TYPE_NATIVE1V; }
    if (tag == 0x06) { meta.type = CELLS_NODE_TYPE_NATIVE2V; }
    goto check_occupied;
  }

  if (b == 0x81) {
    meta.type = CELLS_NODE_TYPE_SEQ;
    meta.size = 1;
    goto check_occupied;
  }
  if (b == 0x82) {
    meta.type = CELLS_NODE_TYPE_SET;
    meta.size = 1;
    goto check_occupied;
  }
  if (b == 0x83) {
    meta.type = CELLS_NODE_TYPE_LAMBDA;
    meta.size = 1;
    goto check_occupied;
  }

check_occupied:
  for (size_t i = index; i < index + meta.size; i++) {
    if (_bitmap_get_bit(cells->occupied_bitmap, i) != 1) {
      meta.type = CELLS_NODE_TYPE_INVALID;
      meta.size = 0;
      break;
    }
  }
  return meta;
}

struct cells_node_t cells_get_node(
    struct cells_t* cells, size_t index, struct cells_node_meta_t meta) {
  struct cells_node_t node = {0};
  if (index > cells->capacity) { return node; }

  switch (meta.type) {
    case CELLS_NODE_TYPE_REF1:
    case CELLS_NODE_TYPE_REF2:
    case CELLS_NODE_TYPE_REF4:
    case CELLS_NODE_TYPE_REF8: {
      i64 offset = 0;
      if (read_signed_int(cells->data, cells->capacity, index, meta, &offset) < 0) { return node; }
      node.meta = meta;
      node.as.ref = offset;
      return node;
    }
    case CELLS_NODE_TYPE_TREE0:
    case CELLS_NODE_TYPE_TREE1:
    case CELLS_NODE_TYPE_TREE2: {
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_NATIVE0F:
    case CELLS_NODE_TYPE_NATIVE1F:
    case CELLS_NODE_TYPE_NATIVE2F: {
      if (cells->data[index] != 0x80) { return node; }
      i64 value = 0;
      if (read_signed_int(cells->data, cells->capacity, index + 1, meta, &value) < 0) {
        return node;
      }
      node.meta = meta;
      node.as.nativef = value;
      return node;
    }
    case CELLS_NODE_TYPE_NATIVE0V:
    case CELLS_NODE_TYPE_NATIVE1V:
    case CELLS_NODE_TYPE_NATIVE2V: {
      if (cells->data[index] != 0x80) { return node; }
      span_byte_t payload;
      if (read_payload(cells->data, cells->capacity, index + 1, meta, &payload) < 0) {
        return node;
      }
      node.as.nativev = payload;
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_SEQ:
    case CELLS_NODE_TYPE_SET:
    case CELLS_NODE_TYPE_LAMBDA: {
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_INVALID:
      // TODO: report here, stacktrace + node data
      return node;
  }
  return node;
}

int cells_alloc_node(struct cells_t* cells, size_t node_size, size_t* index_out) {
  // to keep next index close to the start of the memory, cap it to part of capacity
  size_t start_index = cells->next_free_index % (cells->capacity / 2);
  size_t free_bytes_count = 0;
  for (size_t i = start_index; i < cells->capacity; i++) {
    if (_bitmap_get_bit(cells->occupied_bitmap, i) == 0) {
      free_bytes_count++;
    } else {
      free_bytes_count = 0;
    }

    if (free_bytes_count == node_size) {
      *index_out = i - free_bytes_count + 1;
      for (size_t j = *index_out; j < *index_out + node_size; j++) {
        _bitmap_set_bit(cells->occupied_bitmap, j, 1);
      }
      cells->next_free_index = *index_out + node_size;
      return 0;
    }
  }
  return -1;
}

int cells_write_node(struct cells_t* cells, size_t index, struct cells_node_t node) {
  switch (node.meta.type) {
    case CELLS_NODE_TYPE_REF1:
    case CELLS_NODE_TYPE_REF2:
    case CELLS_NODE_TYPE_REF4:
    case CELLS_NODE_TYPE_REF8:
      return write_signed_int(cells->data, cells->capacity, index, node.meta, node.as.ref);

    case CELLS_NODE_TYPE_TREE0:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0xc0;
      return 0;

    case CELLS_NODE_TYPE_TREE1:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0xe0;
      return 0;

    case CELLS_NODE_TYPE_TREE2:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0xd0;
      return 0;
    case CELLS_NODE_TYPE_NATIVE0F:
    case CELLS_NODE_TYPE_NATIVE1F:
    case CELLS_NODE_TYPE_NATIVE2F:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0x80;
      return write_signed_int(cells->data, cells->capacity, index + 1, node.meta, node.as.nativef);

    case CELLS_NODE_TYPE_NATIVE0V:
    case CELLS_NODE_TYPE_NATIVE1V:
    case CELLS_NODE_TYPE_NATIVE2V: {
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0x80;
      return write_payload(cells->data, cells->capacity, index + 1, node);
    }
    case CELLS_NODE_TYPE_SEQ:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0x81;
      return 0;
    case CELLS_NODE_TYPE_SET:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0x82;
      return 0;
    case CELLS_NODE_TYPE_LAMBDA:
      if (index + 1 > cells->capacity) { return -1; }
      cells->data[index] = 0x83;
      return 0;

    case CELLS_NODE_TYPE_INVALID:
      // TODO: report here, stacktrace + node data
      assert(0 && "invalid node");
  }
}

int cells_node_free(struct cells_t* cells, size_t index, size_t node_size) {
  for (size_t i = index; i < index + node_size; ++i) {
    cells->data[i] = 0;
    _bitmap_set_bit(cells->occupied_bitmap, i, 0);
  }
  cells->next_free_index = index % (cells->capacity / 2);
  return 0;
}

struct cells_node_t cells_new_ref1(i8 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF1;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref2(i16 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF2;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref4(i32 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF4;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref8(i64 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF8;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_tree0() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE0;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_tree1() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE1;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_tree2() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE2;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_native0f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE0F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native1f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE1F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native2f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE2F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native0v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE0V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_native1v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE1V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_native2v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE2V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}
