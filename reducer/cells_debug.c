#include "cells_api.h"
#include "cells_impl.h"
#include "domain_api.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#define COLOR_RESET           "\033[0m"
#define COLOR_RED             "\033[31m"
#define COLOR_GREEN           "\033[32m"
#define COLOR_YELLOW          "\033[33m"
#define COLOR_BLUE            "\033[34m"
#define COLOR_MAGENTA         "\033[35m"
#define COLOR_CYAN            "\033[36m"
#define COLOR_GREY            "\033[90m"
#define COLOR_GREY_UNDERLINED "\033[4;37m"

static const char* get_node_color(cells_node_type_t type) {
  enum cells_node_layout_t layout = cells_node_type_get_layout(type);
  if (cells_node_type_is_ref(type)) { return COLOR_CYAN; }
  if (type.value == CELLS_NODE_TYPE_DELTA0 || type.value == CELLS_NODE_TYPE_DELTA1
      || type.value == CELLS_NODE_TYPE_DELTA2) {
    return COLOR_GREEN;
  }
  if (type.value == CELLS_NODE_TYPE_APPLY) { return COLOR_MAGENTA; }
  if (layout == CELLS_NODE_LAYOUT_TAG) { return COLOR_BLUE; }
  if (layout == CELLS_NODE_LAYOUT_I64) { return COLOR_YELLOW; }
  if (layout == CELLS_NODE_LAYOUT_BYTES) { return COLOR_GREY_UNDERLINED; }
  return COLOR_RED;
}

static void print_node_desc(
    struct cells_t* cells,
    size_t index,
    struct cells_node_header_t header,
    cells_print_fn print,
    void* ctx) {
  struct cells_node_t node = cells_get_node(cells, index, header);
  enum cells_node_layout_t layout = cells_node_type_get_layout(header.type);
  i8 arity = cells_node_type_get_arity(header.type);
  const char* color = get_node_color(header.type);
  print(ctx, "%s", color);
  print(ctx, "[%zu]", index);
  if (cells_node_type_is_ref(header.type)) {
    print(ctx, "*%zu{%" PRId64 "}", header.encoded_size, node.as.ref + index);
  } else if (
      header.type.value == CELLS_NODE_TYPE_DELTA0 || header.type.value == CELLS_NODE_TYPE_DELTA1
      || header.type.value == CELLS_NODE_TYPE_DELTA2) {
    print(ctx, "Δ%d", arity);
  } else if (layout == CELLS_NODE_LAYOUT_I64) {
    print(ctx, "v%df{%" PRId64 "}", arity, node.as.nativef);
  } else if (layout == CELLS_NODE_LAYOUT_BYTES) {
    print(ctx, "v%dv{len=%" PRIu64, arity, node.as.nativev.len);
    if (node.as.nativev.len > 0) {
      print(ctx, ", data=0x");
      size_t print_len = node.as.nativev.len > 4 ? 4 : node.as.nativev.len;
      for (size_t i = 0; i < print_len; ++i) {
        print(ctx, "%02x", node.as.nativev.data[i]);
      }
      if (node.as.nativev.len > 4) { print(ctx, "..."); }
    }
    print(ctx, "}");
  } else {
    print(ctx, "%s", cells_node_type_t_str(header.type));
  }
  print(ctx, "%s", COLOR_RESET);
}

void cells_print_debug_view(struct cells_t* cells, cells_print_fn print, void* ctx) {
  const size_t BYTES_PER_ROW = 16;
  size_t capacity = cells->capacity;

  size_t current_node_end = 0;
  struct cells_node_header_t current_header = {0};
  // We use a dummy type for free space to distinguish from INVALID
  const cells_node_type_t TYPE_FREE = {.value = (u8)-1};
  current_header.type = TYPE_FREE;

  print(ctx, "MAHNODES  ");
  for (size_t i = 0; i < BYTES_PER_ROW; ++i) {
    print(ctx, "%02zu ", i);
  }
  print(ctx, " |\n");

  for (size_t row_start = 0; row_start < capacity; row_start += BYTES_PER_ROW) {
    // 1. Print Offset
    print(ctx, "%s%08zu%s  ", COLOR_GREY, row_start, COLOR_RESET);

    // Store indices of nodes starting in this row
    size_t nodes_starting_in_row[BYTES_PER_ROW];
    size_t nodes_count = 0;

    // 2. Print Hex
    for (size_t i = 0; i < BYTES_PER_ROW; ++i) {
      size_t idx = row_start + i;
      if (idx >= capacity) {
        print(ctx, "   ");
        continue;
      }

      // Check if we are at the start of a new node or in free space
      if (idx >= current_node_end) {
        if (_bitmap_get_bit(cells->occupied_bitmap, idx)) {
          // New node starts here
          current_header = cells_get_node_header(cells, idx);
          if (current_header.type.value != CELLS_NODE_TYPE_INVALID) {
            current_node_end = idx + current_header.encoded_size;
            nodes_starting_in_row[nodes_count++] = idx;
          } else {
            // Invalid node (occupied but parse failed or invalid type)
            current_node_end = idx + 1; // Treat as 1 byte
            current_header.type.value = CELLS_NODE_TYPE_INVALID;
            nodes_starting_in_row[nodes_count++] = idx;
          }
        } else {
          // Free space
          current_node_end = idx + 1;
          current_header.type = TYPE_FREE;
        }
      }

      // Determine color
      const char* color = COLOR_GREY;
      if (current_header.type.value != TYPE_FREE.value) {
        color = get_node_color(current_header.type);
      }

      print(ctx, "%s%02x%s ", color, cells->data[idx], COLOR_RESET);
    }

    print(ctx, " | ");

    // 3. Print Descriptions
    for (size_t i = 0; i < nodes_count; ++i) {
      size_t idx = nodes_starting_in_row[i];
      struct cells_node_header_t header = cells_get_node_header(cells, idx);
      // If it was invalid during scan, it might be invalid here too.
      if (header.type.value == CELLS_NODE_TYPE_INVALID
          && _bitmap_get_bit(cells->occupied_bitmap, idx)) {
        // It was occupied but invalid
        header.type.value = CELLS_NODE_TYPE_INVALID;
        header.encoded_size = 1;
      }

      print_node_desc(cells, idx, header, print, ctx);
      if (i < nodes_count - 1) { print(ctx, " "); }
    }

    print(ctx, "\n");
  }
}
