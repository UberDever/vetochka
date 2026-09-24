# Tree as a byte stream with inline payload

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- EPIC: 20241225-125809

- Decision (user, 2026-02-01): the tree is a byte stream with payload inline, since code is data and may change itself.
- Refs are variable size. Tree nodes start with `11`, natives and opcodes with `10`.
- Refined (user, 2026-02-09): only `ref2` and `ref8` remain.
