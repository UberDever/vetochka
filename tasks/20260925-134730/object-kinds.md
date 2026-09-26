# Runtime object kinds (2026-09-25)

Grammar first, then execution rules. Sources: `docs/new_spec/02_syntax.md` (rewrite rules),
`docs/new_spec/03_v0.md`, and the rulings cited per row. This is an inventory, not a design.

## Kinds that come from the grammar

| Kind | Lowered form today | Execution rule | Source of the rule |
|-|-|-|-|
| nyad | `~[]`, `~[x]`, `~[x, y]`; also list cells | triage rules 0a..3c | spec, 03_v0 |
| integer | native i64 leaf | none written | open |
| byte string | native bytes leaf | none written; meaning comes from the parent node | (user, 2026-09-24) |
| application | `[{@}, meta, f, x]` (rules 5, 9, 10, loose postfix, opcode) | only through `exec`, then callee-first in the machine | (user, 2026-08-30); July `RUN-APPLY` |
| identifier | `[{:id}, meta, {x}]` | dereference; unbound is an uncatchable trap; observation never errors | (user, 2026-08-30) |
| opcode head | `[{:id}, meta, {$}]` (rule 2) | selects an opcode protocol when applied to a label | spec, 03_v0 |
| label | `[{:label}, meta, {label}, expr]` | none of its own; opcodes read it as input; as a callee it traps | (2026-07-11), not restated since |
| block | `[{:block}, meta, a, b]` | none of its own; `$fn` runs it as statements | spec; (user, 2026-08-30) sequencing B |
| annotation | `[{:annot}, meta, list, expr]` | none in v0 | vf |
| prefix | `[{:prefix}, meta, {op}, expr]` | none in v0 | vf |
| infix | `[{:infix}, meta, [ops], operands...]` | none in v0 | vf |
| selector | `[{:selector}, meta, base, {name}]` | none in v0 | vf |

## Runtime objects with no grammar form

Runtime objects are first-class: a program can store and pass them (user, 2026-09-25).

| Kind | Origin | Execution rule | Observation | Source |
|-|-|-|-|-|
| opcode state (code closure) | `$` applied to labels, partially | `Continue(next-state, inputs)`, `Dispatch(result)`, hard trap | pith: selected opcode and inputs as written | spec; (user, 2026-08-30) |
| closure | `$fn` given its body | saturation runs the body under the declaration env | pith: declarators and body syntax, no env | (user, 2026-08-30) |
| pith / view | Rule 3 on a node | stateful view; Rule 3 advances it; can be held in a variable | itself | (user, 2026-09-25); July LENS: unforgeable, non-callable |
| native handle | host values | none in v0 | minimal pith, e.g. an integer | (user, 2026-09-24) |

## Machine state, not objects

Nothing in the language can hold these; they exist only while the machine runs.

- Pending application: what `exec` makes of `{@}` data; callee-first in July's `RUN-APPLY`.
- TERM: a term paired with its env for deferred evaluation (user, 2026-08-30).

## Reading (agent, not ruled)

- Only some grammar kinds have execution rules of their own: nyad, application, identifier and
  opcode head. Integers and byte strings have none written. Label and block are opcode inputs. Annotation, prefix, infix and
  selector are data for vf.
- Under the user's framing (tags for semantic objects, not for storage layouts), the candidates for
  their own cell tags are the kinds with execution rules plus the run-time kinds. The vf-data kinds
  can stay `:`-headed lists.
- Open: integers and byte strings have no execution rule written anywhere.
