# Current cells encoding (2026-09-25)

Source: `reducer/cells_impl.h` (`CELLS_NODE_INFO_ITEMS`), `reducer/cells_api.h`, `reducer/cells_node.c`.
The first byte selects the node: `(byte & mask) == code`. Mask/code/layout pairs are the bytecode ABI.

| Type | First byte | Layout | Size | Arity | Payload | Used today |
|-|-|-|-|-|-|-|
| `DELTA0` | `0x80` | tag | 1 | 0 | none | nyad `~[]` |
| `DELTA1` | `0x81` | tag | 1 | 1 | none | nyad `~[x]` |
| `DELTA2` | `0x82` | tag | 1 | 2 | none | nyad `~[x, y]`, list cells |
| `VALUEF0` | `0x83` | i64 | 9 | 0 | i64, little-endian | integer literals |
| `VALUEF1` | `0x84` | i64 | 9 | 1 | i64 | reducer tests only |
| `VALUEF2` | `0x85` | i64 | 9 | 2 | i64 | reducer tests only |
| `VALUEV0` | `0x86` | bytes | 1 + ULEB128 + n | 0 | ULEB128 length, bytes | byte strings, tags `{:id}`, `{@}` |
| `VALUEV1` | `0x87` | bytes | 1 + ULEB128 + n | 1 | length, bytes | reducer tests only |
| `VALUEV2` | `0x88` | bytes | 1 + ULEB128 + n | 2 | length, bytes | reducer tests only |
| `APPLY` | `0x89` | tag | 1 | 2 | none | no producer; the reducer dispatches it by arity (task 20260718-175927) |
| `OP_FN0` | `0x8A` | tag | 1 | 0 | none | old experiment; reducer tests only |
| `OP_FN1` | `0x8B` | tag | 1 | 1 | none | old experiment |
| `OP_FN2` | `0x8C` | tag | 1 | 2 | none | old experiment |
| `REF` (14) | `00xxxxxx` | ref14 | 2 | none | signed 14-bit offset, big-endian | child links |
| `REF` (62) | `01xxxxxx` | ref62 | 8 | none | signed 62-bit offset, big-endian | child links |

- Children follow their parent. For arity 2 the left child slot must be a `REF`, and the right
  child follows it. A ref's offset is added to the ref's own index (`cells_dereference_node`); refs chain.
- Arity steps within a family through `NEXT_TYPE`: `DELTA0 -> DELTA1 -> DELTA2`, the same for
  `VALUEF`, `VALUEV`, `OP_FN`. `cells_node_set_arity` uses it.
- The C API identities (`CELLS_NODE_TYPE_*`, `0x00..0x0D`, `REF 0xF0`) differ from the wire
  bytes above.
