# Canonical i64: code-generation experiment

Question: can a canonical eight-octet little-endian payload coexist with simple native arithmetic?

Commands:

```sh
clang -O2 -S i64-probe.c -o x86-64.s
clang --target=aarch64-none-elf -O2 -S i64-probe.c -o aarch64.s
```

Observed: both targets compile the bytewise little-endian loader into a single 64-bit load.
Wrapping unsigned addition uses one native arithmetic instruction. Checked signed addition uses native addition and overflow-condition extraction, plus storing the result.
The source uses unsigned arithmetic for wrapping and a compiler builtin for checked arithmetic.

Scope: these are compiler outputs, not runtime measurements. They do not measure tag checks, allocation, blob inspection, aliasing, dispatch, or a complete Vetochka operation.
No runtime implementation or arithmetic policy was selected. The implementation may optimize representation only while preserving language-visible behavior.

User direction: favor canonical i64 representation with fixed little-endian byte order and deterministic operation behavior.
Agent proposal: use exactly eight octets with two's-complement interpretation. Overflow policy, exceptional division, shifts, conversions, and malformed payload handling remain open.
Canonical payloads do not imply canonical complete tagged trees when provenance differs.
