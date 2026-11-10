# Claude opcode / `do!` research — 2026-07-10

Source: `/home/uberdever/.claude/projects/-home-uberdever-dev-c-vetochka/ee8239ae-cecd-4008-b939-dc7f08829961.jsonl` around messages 427–443.
Status: historical design-research artifact; design hint, not authority. Imported 2026-07-11.

## User model from conversation

- Opcodes are facility nodes like `REF`/`APPLY`, not triage-calculus objects.
- Relation of non-triage nodes to calculus should be arity/storage/execution protocol, not full triage leaf/stem/fork identity.
- Opcodes should not be modeled as ordinary leaf/stem/fork values; better model is unsaturated/saturated/complete opcode states.
- Opcode is a state machine: `opcode0 -> opcode1 -> ... -> opcodeN`; effect/action occurs only when complete.
- Opcode transitions may branch based on arguments, e.g. `do! fn:` receiving `expr:` vs `do:` can move to different states/protocols.
- General `{do}` runtime dispatch was tempting but problematic because runtime content-dispatch breaks static name resolution / specialization.
- Later user refinement: opcode is always observed as a stem; reducer cannot observe opcode fork because completion executes immediately.
- `do!` can be the source-level mechanism for opcode construction/protocol selection, replacing string-application `{fn}` style.
- Runtime should keep dynamic opcode state label (state-machine label), not necessarily a simple counter.

## Claude assessment captured then

- Useful distinction: saturated is not necessarily complete.
- One opcode node kind plus `(family,state)` payload is simpler than per-arity node triples like `OP_FN0/1/2`.
- `APPLY shapeless` can be understood as demand-driven forcing/evaluation, not a special exception to storage arity.
- Shape inspection and opcode completion need reconciliation: candidate was shape projection of state (`complete ? fork : stem`) so rule 3 remains total, while machine keeps richer state graph.
- Strong crux then: whether opcode state graph lives in specialization-time protocol recognition or runtime machine dispatch.
- Claude leaned toward static topology + runtime saturation/state progression to preserve static name resolution.

## Relevance to 2026-07-11 spec work

- Supports current direction that `OPCODE(state)` is a real executable node family, with state identifying protocol/kind/transition.
- Supports keeping `do!` as source structure / executable-position protocol, not runtime opcode itself.
- Supports TODO: exact `fn` opcode shape/state machine must be specified later, outside current layer pass.
- Must be reinterpreted after 2026-07-11 decision removing user-visible `{specialize}`: specialization is activation-time, not explicit compile opcode/function.
