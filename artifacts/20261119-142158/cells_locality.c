/* Throwaway native traversal experiment for the cells layout (19.11.26, 4). */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error This experiment uses native little-endian loads.
#endif

enum { FORK = 1, INDIRECT = 3, INLINE_FRUIT = 4, KIND_MASK = 7 };
enum { LEFT = 0, RIGHT = 1, SHARED = 2, MAX_TRIALS = 21 };

struct root {
    uint32_t offset;
    uint32_t base;
};

struct heap {
    uint8_t *bytes;
    struct root *roots;
    uint32_t used;
    uint32_t long_refs;
    unsigned width;
    bool grouped;
};

struct result {
    uint64_t checksum;
    uint64_t visits;
    uint64_t hops;
};

static void *allocate(size_t size) {
    void *memory = malloc(size);
    if (memory == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return memory;
}

static uint32_t random_word(uint32_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static uint32_t *permutation(uint32_t count, uint32_t seed) {
    uint32_t *order = allocate((size_t)count * sizeof(*order));
    for (uint32_t index = 0; index < count; ++index) {
        order[index] = index;
    }
    for (uint32_t index = count - 1; index > 0; --index) {
        uint32_t other = random_word(&seed) % (index + 1);
        uint32_t value = order[index];
        order[index] = order[other];
        order[other] = value;
    }
    return order;
}

static unsigned marker(uint32_t family, unsigned fragment) {
    uint32_t seed = (family + 1) * UINT32_C(0x9e3779b9) + fragment;
    return 1 + random_word(&seed) % 255;
}

static uint64_t short_word(unsigned kind, uint32_t target, uint32_t base, unsigned width) {
    assert(target >= base && target - base < (UINT32_C(1) << (8 * width - 3)));
    return ((uint64_t)(target - base) << 3) | kind;
}

static uint32_t add_long_ref(struct heap *heap, unsigned kind, uint32_t target) {
    uint32_t location = heap->used;
    int64_t displacement = (int64_t)target - location;
    unsigned width = 2 * heap->width;
    int64_t limit = INT64_C(1) << (8 * width - 4);
    assert(-limit <= displacement && displacement < limit);
    uint64_t word = ((uint64_t)displacement << 3) | kind;
    memcpy(heap->bytes + location, &word, width);
    heap->used += width;
    ++heap->long_refs;
    return location;
}

static void add_fragment(struct heap *heap, uint32_t family, unsigned fragment,
                         unsigned span, uint32_t base, uint64_t last_right) {
    uint64_t fruit = ((uint64_t)marker(family, fragment) << 8) | (1 << 3) | INLINE_FRUIT;
    unsigned width = heap->width;
    for (unsigned index = 0; index < span; ++index) {
        uint32_t offset = heap->used;
        uint64_t left = index % 8 == 0 ? fruit : 0;
        uint64_t right = index + 1 == span
                         ? last_right : short_word(FORK, offset + 2 * width, base, width);
        memcpy(heap->bytes + offset, &left, width);
        memcpy(heap->bytes + offset + width, &right, width);
        heap->used += 2 * width;
    }
}

static void place_grouped(struct heap *heap, uint32_t families, unsigned span) {
    uint32_t base = 0;
    uint32_t fragment_bytes = 2 * heap->width * span;
    uint32_t reach = UINT32_C(1) << (8 * heap->width - 3);
    for (uint32_t family = 0; family < families; ++family) {
        if (heap->used - base + 3 * fragment_bytes > reach) {
            base = heap->used;
        }
        uint32_t start = heap->used;
        uint64_t to_shared = short_word(FORK, start + fragment_bytes, base, heap->width);
        heap->roots[2 * family] = (struct root){ start, base };
        add_fragment(heap, family, LEFT, span, base, to_shared);
        add_fragment(heap, family, SHARED, span, base, 0);
        heap->roots[2 * family + 1] = (struct root){ heap->used, base };
        add_fragment(heap, family, RIGHT, span, base, to_shared);
    }
}

static void place_interleaved(struct heap *heap, uint32_t families, unsigned span,
                              const uint32_t *suffix_order, bool indirect) {
    uint32_t *shared = allocate((size_t)families * sizeof(*shared));
    uint32_t record_bytes = 2 * heap->width;
    uint32_t fragment_bytes = span * record_bytes;

    for (uint32_t index = 0; index < families; ++index) {
        uint32_t family = suffix_order[index];
        uint32_t base = indirect ? heap->used : 0;
        shared[family] = heap->used;
        if (indirect) {
            add_long_ref(heap, FORK, base + record_bytes);
        }
        add_fragment(heap, family, SHARED, span, base, 0);
    }

    for (unsigned fragment = LEFT; fragment <= RIGHT; ++fragment) {
        for (uint32_t family = 0; family < families; ++family) {
            uint32_t start = heap->used;
            uint32_t base = indirect ? start : 0;
            heap->roots[2 * family + fragment] = (struct root){ start, base };
            if (indirect) {
                uint32_t bridge = start + fragment_bytes;
                add_fragment(heap, family, fragment, span, base,
                             short_word(INDIRECT, bridge, base, heap->width));
                add_long_ref(heap, INDIRECT, shared[family]);
            } else {
                add_fragment(heap, family, fragment, span, base,
                             short_word(FORK, shared[family], base, heap->width));
            }
        }
    }
    free(shared);
}

static struct heap build(unsigned width, bool grouped, uint32_t families, unsigned span,
                         const uint32_t *suffix_order) {
    size_t fork_bytes = (size_t)families * 3 * span * 2 * width;
    bool indirect = !grouped && fork_bytes > (UINT32_C(1) << (8 * width - 3));
    size_t length = fork_bytes + (indirect ? (size_t)families * 3 * 2 * width : 0);
    struct heap heap = {
        .bytes = allocate(length),
        .roots = allocate((size_t)families * 2 * sizeof(struct root)),
        .width = width,
        .grouped = grouped,
    };
    if (grouped) {
        place_grouped(&heap, families, span);
    } else {
        place_interleaved(&heap, families, span, suffix_order, indirect);
    }
    assert(heap.used == length);
    return heap;
}

static inline uint64_t load_word(const uint8_t *address, unsigned width) {
    if (width == 8) {
        uint64_t word;
        memcpy(&word, address, sizeof(word));
        return word;
    }
    if (width >= 4) {
        uint32_t low;
        memcpy(&low, address, sizeof(low));
        if (width == 4) {
            return low;
        }
        uint16_t high;
        memcpy(&high, address + 4, sizeof(high));
        return low | ((uint64_t)high << 32);
    }
    uint16_t low;
    memcpy(&low, address, sizeof(low));
    return width == 2 ? low : low | ((uint32_t)address[2] << 16);
}

static inline uint64_t add_root_checksum(uint64_t checksum, uint64_t root_sum) {
    return (checksum ^ root_sum) * UINT64_C(1099511628211);
}

/* Specialize the loads for each width and leave hop counting out of the timed walk. */
static inline __attribute__((always_inline)) struct result walk(
        const struct heap *heap, const uint32_t *root_order, uint32_t root_count,
        unsigned width, bool count_hops) {
    struct result result = {0};
    for (uint32_t index = 0; index < root_count; ++index) {
        struct root root = heap->roots[root_order[index]];
        uint32_t offset = root.offset;
        uint32_t base = root.base;
        uint64_t sum = 0;
        do {
            const uint8_t *fork = heap->bytes + offset;
            sum += load_word(fork, width) >> 8;
            uint64_t right = load_word(fork + width, width);
            ++result.visits;
            if (right == 0) {
                break;
            }
            offset = base + (uint32_t)(right >> 3);
            while ((right & KIND_MASK) == INDIRECT) {
                base = offset;
                right = load_word(heap->bytes + base, 2 * width);
                int64_t sign = INT64_C(1) << (16 * width - 4);
                int64_t displacement = (int64_t)((right >> 3) ^ (uint64_t)sign) - sign;
                offset = (uint32_t)((int64_t)base + displacement);
                if (count_hops) {
                    ++result.hops;
                }
            }
        } while (true);
        result.checksum = add_root_checksum(result.checksum, sum);
    }
    return result;
}

static __attribute__((noinline)) struct result run(
        const struct heap *heap, const uint32_t *order, uint32_t count, bool profile) {
    switch (heap->width) {
    case 2:
        return profile ? walk(heap, order, count, 2, true) : walk(heap, order, count, 2, false);
    case 3:
        return profile ? walk(heap, order, count, 3, true) : walk(heap, order, count, 3, false);
    case 4:
        return profile ? walk(heap, order, count, 4, true) : walk(heap, order, count, 4, false);
    }
    abort();
}

static uint64_t expected_checksum(const uint32_t *order, uint32_t count, unsigned span) {
    uint64_t checksum = 0;
    for (uint32_t index = 0; index < count; ++index) {
        uint32_t root = order[index];
        unsigned sum = marker(root / 2, root % 2) + marker(root / 2, SHARED);
        checksum = add_root_checksum(checksum, sum * ((span + 7) / 8));
    }
    return checksum;
}

static uint64_t nanoseconds(void) {
    struct timespec tick;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (uint64_t)tick.tv_sec * UINT64_C(1000000000) + (uint64_t)tick.tv_nsec;
}

static int compare_samples(const void *left, const void *right) {
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;
    return (a > b) - (a < b);
}

static unsigned argument(const char *text, unsigned maximum) {
    char *end = NULL;
    unsigned long number = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || number < 1 || number > maximum) {
        fprintf(stderr, "expected an integer in [1, %u], got %s\n", maximum, text);
        exit(EXIT_FAILURE);
    }
    return (unsigned)number;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s FAMILIES FORKS_PER_FRAGMENT TRIALS\n", argv[0]);
        return EXIT_FAILURE;
    }
    uint32_t families = argument(argv[1], 1u << 20);
    unsigned span = argument(argv[2], 256);
    unsigned trials = argument(argv[3], MAX_TRIALS);
    if ((uint64_t)families * span > (1u << 22)) {
        fprintf(stderr, "this experiment supports up to 12,582,912 distinct forks\n");
        return EXIT_FAILURE;
    }
    uint32_t roots = 2 * families;
    uint64_t visits = (uint64_t)roots * 2 * span;
    uint32_t *suffix_order = permutation(families, UINT32_C(0x12345678));
    uint32_t *root_order = permutation(roots, UINT32_C(0x87654321));
    uint64_t expected = expected_checksum(root_order, roots, span);
    struct heap heaps[6];
    struct result profiles[6];
    uint64_t samples[6][MAX_TRIALS];

    for (unsigned index = 0; index < 6; ++index) {
        heaps[index] = build(2 + index / 2, index % 2 == 0, families, span, suffix_order);
        profiles[index] = run(&heaps[index], root_order, roots, true);
        assert(profiles[index].checksum == expected && profiles[index].visits == visits);
        uint64_t expected_hops = heaps[index].long_refs ? (uint64_t)roots * 2 : 0;
        assert(profiles[index].hops == expected_hops);
    }
    for (unsigned trial = 0; trial < trials; ++trial) {
        for (unsigned order = 0; order < 6; ++order) {
            unsigned index = (order + trial) % 6;
            uint64_t start = nanoseconds();
            struct result result = run(&heaps[index], root_order, roots, false);
            samples[index][trial] = nanoseconds() - start;
            assert(result.checksum == expected && result.visits == visits);
        }
    }

    printf("families=%u span=%u trials=%u distinct_forks=%llu visits=%llu root_bytes=%zu\n",
           families, span, trials, (unsigned long long)families * span * 3,
           (unsigned long long)visits, (size_t)roots * sizeof(struct root));
    for (unsigned index = 0; index < 6; ++index) {
        struct heap *heap = &heaps[index];
        qsort(samples[index], trials, sizeof(samples[index][0]), compare_samples);
        unsigned middle = trials / 2;
        double median = ((double)samples[index][middle] + samples[index][(trials - 1) / 2]) / 2;
        printf("w=%u %s bytes=%u long_refs=%u hops=%llu cpu-ns/fork=%.3f range=%.3f–%.3f\n",
               heap->width, heap->grouped ? "grouped" : "interleaved", heap->used,
               heap->long_refs, (unsigned long long)profiles[index].hops, median / visits,
               (double)samples[index][0] / visits, (double)samples[index][trials - 1] / visits);
        free(heap->bytes);
        free(heap->roots);
    }
    free(suffix_order);
    free(root_order);
    return EXIT_SUCCESS;
}
