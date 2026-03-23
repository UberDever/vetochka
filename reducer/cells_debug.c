#include "cells_api.h"
#include "cells_impl.h"
#include "typedefs.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_GREY    "\033[90m"

static const char* get_node_color(enum CELLS_NODE_TYPE type) {
  switch (type) {
    case CELLS_NODE_TYPE_REF2:
    case CELLS_NODE_TYPE_REF8: return COLOR_CYAN;
    case CELLS_NODE_TYPE_DELTA0:
    case CELLS_NODE_TYPE_DELTA1:
    case CELLS_NODE_TYPE_DELTA2: return COLOR_GREEN;
    case CELLS_NODE_TYPE_VALUEF0:
    case CELLS_NODE_TYPE_VALUEF1:
    case CELLS_NODE_TYPE_VALUEF2: return COLOR_YELLOW;
    case CELLS_NODE_TYPE_VALUEV0:
    case CELLS_NODE_TYPE_VALUEV1:
    case CELLS_NODE_TYPE_VALUEV2: return COLOR_MAGENTA;
    case CELLS_NODE_TYPE_INVALID: return COLOR_RED;
    default: return COLOR_RED;
  }
}

static void print_node_desc(
    struct cells_t* cells,
    size_t index,
    struct cells_node_meta_t meta,
    cells_print_fn print,
    void* ctx) {
  struct cells_node_t node = cells_get_node(cells, index, meta);
  const char* color = get_node_color(meta.type);
  print(ctx, "%s", color);
  print(ctx, "[%zu]", index);
  switch (meta.type) {
    case CELLS_NODE_TYPE_REF2:
    case CELLS_NODE_TYPE_REF8:
      print(ctx, "*%zu{%" PRId64 "}", meta.size, node.as.ref + index);
      break;
    case CELLS_NODE_TYPE_DELTA0: print(ctx, "Δ0"); break;
    case CELLS_NODE_TYPE_DELTA1: print(ctx, "Δ1"); break;
    case CELLS_NODE_TYPE_DELTA2: print(ctx, "Δ2"); break;
    case CELLS_NODE_TYPE_VALUEF0:
    case CELLS_NODE_TYPE_VALUEF1:
    case CELLS_NODE_TYPE_VALUEF2: {
      int n = 0;
      if (meta.type == CELLS_NODE_TYPE_VALUEF1) {
        n = 1;
      } else if (meta.type == CELLS_NODE_TYPE_VALUEF2) {
        n = 2;
      }
      print(ctx, "v%df{%" PRId64 "}", n, node.as.nativef);
      break;
    }
    case CELLS_NODE_TYPE_VALUEV0:
    case CELLS_NODE_TYPE_VALUEV1:
    case CELLS_NODE_TYPE_VALUEV2: {
      int n = 0;
      if (meta.type == CELLS_NODE_TYPE_VALUEV1) {
        n = 1;
      } else if (meta.type == CELLS_NODE_TYPE_VALUEV2) {
        n = 2;
      }
      print(ctx, "v%dv{len=%" PRIu64, n, node.as.nativev.len);
      if (node.as.nativev.len > 0) {
        print(ctx, ", data=0x");
        size_t print_len = node.as.nativev.len > 4 ? 4 : node.as.nativev.len;
        for (size_t i = 0; i < print_len; ++i) {
          print(ctx, "%02x", node.as.nativev.data[i]);
        }
        if (node.as.nativev.len > 4) { print(ctx, "..."); }
      }
      print(ctx, "}");
      break;
    }
    case CELLS_NODE_TYPE_INVALID: print(ctx, "INVALID"); break;
  }
  print(ctx, "%s", COLOR_RESET);
}

void cells_print_debug_view(struct cells_t* cells, cells_print_fn print, void* ctx) {
  const size_t BYTES_PER_ROW = 16;
  size_t capacity = cells->capacity;

  size_t current_node_end = 0;
  struct cells_node_meta_t current_meta = {0};
  // We use a dummy type for free space to distinguish from INVALID
  const enum CELLS_NODE_TYPE TYPE_FREE = (enum CELLS_NODE_TYPE) - 1;
  current_meta.type = TYPE_FREE;

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
          current_meta = cells_get_node_meta(cells, idx);
          if (current_meta.type != CELLS_NODE_TYPE_INVALID) {
            current_node_end = idx + current_meta.size;
            nodes_starting_in_row[nodes_count++] = idx;
          } else {
            // Invalid node (occupied but parse failed or invalid type)
            current_node_end = idx + 1; // Treat as 1 byte
            current_meta.type = CELLS_NODE_TYPE_INVALID;
            nodes_starting_in_row[nodes_count++] = idx;
          }
        } else {
          // Free space
          current_node_end = idx + 1;
          current_meta.type = TYPE_FREE;
        }
      }

      // Determine color
      const char* color = COLOR_GREY;
      if (current_meta.type != TYPE_FREE) { color = get_node_color(current_meta.type); }

      print(ctx, "%s%02x%s ", color, cells->data[idx], COLOR_RESET);
    }

    print(ctx, " | ");

    // 3. Print Descriptions
    for (size_t i = 0; i < nodes_count; ++i) {
      size_t idx = nodes_starting_in_row[i];
      struct cells_node_meta_t meta = cells_get_node_meta(cells, idx);
      // If it was invalid during scan, it might be invalid here too.
      if (meta.type == CELLS_NODE_TYPE_INVALID && _bitmap_get_bit(cells->occupied_bitmap, idx)) {
        // It was occupied but invalid
        meta.type = CELLS_NODE_TYPE_INVALID;
        meta.size = 1;
      }

      print_node_desc(cells, idx, meta, print, ctx);
      if (i < nodes_count - 1) { print(ctx, " "); }
    }

    print(ctx, "\n");
  }
}
