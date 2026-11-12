/* Throwaway code-generation probe; not a proposed runtime implementation. */
typedef __UINT64_TYPE__ u64;
typedef __INT64_TYPE__ i64;
u64 load_le(const unsigned char *p) {
    u64 value = 0;
    for (unsigned i = 0; i < 8; ++i) value |= (u64)p[i] << (i * 8);
    return value;
}
u64 wrap_add(u64 a, u64 b) { return a + b; }
int checked_add(i64 a, i64 b, i64 *out) { return __builtin_add_overflow(a, b, out); }
