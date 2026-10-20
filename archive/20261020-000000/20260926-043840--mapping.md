# Mapping proposal (agent, 2026-09-26; revised after user answers) — not applied

## Tree

```text
I1 INTENT (new)        One syntax, expressive for people, trivial for programs to analyze and construct
└── 20241220-144435    INITIATIVE  Design the shared syntax and its lowering        (was the syntax epic)
I2 INTENT (new)        Any program can observe and build any program as data, without quotation
├── 20260925-134730    INITIATIVE  Cells revision
└── tasks directly under I2: pith list, tag structure, inspect natives (closed)
I3 INTENT (new)        New languages are defined by programs over a shared executable core
├── 20260529-103914    INITIATIVE  v0 machine: CESK semantics                        (was the machine epic)
├── 20241225-140425    INITIATIVE  Lexical binders as the only nonlocality           (was the binders epic)
├── 20250123-091611    INITIATIVE  General host interaction: opcodes, native bindings, opaque handles (was the opcode epic)
├── 20241225-125809    INITIATIVE  Compact cell storage                              (was the encoding epic)
└── 20260613-151556    INITIATIVE  Host rim and bootstrap                            (was the host epic)
I4 INTENT              = 20260529-103915, kept in place (was the framing epic)
I5 INTENT (new)        Project bookkeeping keeps design knowledge current and findable with little effort
root                   20260805-162237 intro sources (provisional)
```
## Records by new parent

| New parent | Records |
|-|-|
| I1 → syntax initiative 20241220-144435 | 20260212-163430 numbers · 20260215-122343 curly-infix ✗ · 20260216-100019 s prefix · 20260405-181501 EBNF · 20260601-084008 do-blocks ✗ · 20260606-141936 ASI · 20260831-155152 paren-statements ✗ · 20260924-092637 mixed infix · 20260924-120458 parser update · 20260924-123410 Unicode identifiers · 20260903-085028 metadata list · 20260924-111836 k-v lists |
| I2 directly | 20260805-162238 pith list · 20260209-162003 tag structure · 20260201-183607 inspect natives |
| I2 → cells revision 20260925-134730 | 20260924-100255 reword cells · 20260924-105828 first-class heads |
| I3 → v0 machine 20260529-103914 | 20260529-103916 · 20260529-103917 · 20260606-141937 · 20260613-151555 · 20260711-131257 CESK rules · 20260711-131258 · 20260711-131259 · reducer (superseded by the CESK): 20260201-183606 ✗ · 20260215-122341 · 20260217-074816 · 20260614-140231 · 20260718-175927 |
| I3 → binders 20241225-140425 | 20260604-111747 · 20260606-141939 · 20260711-131256 · 20260805-162239 · 20260830-111134 · 20260830-165615 · 20260830-165616 |
| I3 → host interaction 20250123-091611 | (no children today; the designator ruling lives here) |
| I3 → cell storage 20241225-125809 | 20260201-183605 ✗ · 20260201-183608 · 20260209-185016 · 20260215-122342 · 20260216-130542 ✗ · 20260614-121150 |
| I3 directly | 20260606-141938 C plus Lua |
| I3 → host 20260613-151556 | 20260903-085029 exec |
| I5 | 20260201-183604 log · 20260711-131255 restoration · 20260911-112058 picture experiment · 20260924-074843 tatr move · 20260924-083540 license · 20260926-043840 this restructure |
| root | 20260805-162237 intro sources (provisional) |

✗ = closed as rejected.

## Dissolved epics (closed, claims moved)

| Epic | Becomes | Its unique claims go to |
|-|-|-|
| 20260805-162236 hand-written spec | tag `spec` | process rulings → AGENTS.md; `2026-08-30.md` stays attached here |

The binders, opcode and encoding epics become initiatives (user, 2026-09-26), not tags.

## Duplicated claims: one owner each

| Claim | Now in | Owner | Others get |
|-|-|-|-|
| Rule 3 never evaluates (2026-09-25) | machine epic, CESK, Rule 3 regression, cells revision | v0 initiative | a HUID reference; the regression keeps its code-specific line |
| Opcodes are designator-headed lists (2026-09-26) | opcode epic, cells revision, first-class heads | host interaction initiative 20250123-091611 | reference |
| `exec` is one step (2026-09-26) | exec, CESK | exec | reference |
| Applications carry meta; byte strings don't; meta slot | tag structure, metadata list, parser update | metadata list | reference |
| Every node observable; pith is the bridge | pith list, cells revision | pith list | reference |
| Epic "current state" summaries | every epic | the task that owns each claim | removed from the epic, pointing to the owner |

## Mechanical changes on every record
- `EPIC:` → `PARENT:`; add `KIND:`; drop the `epic` tag.
- Remove `## Tasks` child lists and "Related:" lines; keep `items.md` history where useful.

## Open
- ~~(agent) Host interaction (20250123-091611) overlaps Host rim and bootstrap (20260613-151556): load_file, parse_term, exec, WASI-shaped host API. Merge, or split as bootstrap (how programs start) versus host interaction (how running programs call out)?~~ resolved (user, 2026-09-26): split. Host rim and bootstrap covers how programs start (load_file, parse_term, exec); host interaction covers how running programs call out (opcodes, natives, opaque handles).
