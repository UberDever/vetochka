# Closure design: requirements ledger

Phase: requirements gathering. No closure representation, lowering, or calling convention is selected.
Scope: define the requirements for closures before comparing designs. Product specification files are outside this work.
Method: lightweight dialogue initially; expand only if the unresolved requirements justify it.

## New requirement

- R1 [requirement, user, 2026-11-12]: fast execution of immediately applied curried closures, informed by the earlier ZINC work. The user describes this as reducing curried closures "in place". This does not itself require destructive mutation of shared trees.
- P1 [interpretation, agent]: when an application supplies enough arguments, avoid allocating an intermediate closure for every unary parameter. This is the earlier ZINC-inspired goal, offered as a candidate operational criterion for R1. No allocation bound or timing target is approved yet.

## Established semantic requirements

- R2 [decision, user]: multiple-argument closures mean nested unary closures, not a separate uncurried function kind. Source: (12.11.26, 9).
- R3 [decision, user]: substitution does not force supplied application trees. Bindings explicitly stage computation. Intrinsic reception does not add ambient identifier lookup. Sources: (23.10.26, 3), (05.11.26, 3), (12.11.26, 13).
- R4 [decision, user]: only substituted occurrences are replaced. Other identifiers remain data. Closure production does not imply that every name disappears. Source: (12.11.26, 13-14).
- R5 [constraint, user direction]: binding must preserve intended lexical dependencies. Source spelling is not a required internal identity. Provenance or name-independent lowering are candidates, not selected mechanisms. Sources: (29.07.26, 2), (12.11.26, 14).
- R6 [decision, user]: multiple bindings nest sequentially, so later binding expressions can use earlier bindings. These binders do not supply letrec recursion. Source: (12.11.26, 10). This does not prohibit recursion expressed through other language constructs.
- R7 [decision, user]: a constructed or returned block remains data. Executing a nonempty block forces entries in source order, discards intermediate results, and returns the last result. Source: (26.10.26, 1). Empty-block behavior and the mechanism remain open.
- R8 [decision, user]: proper tail calls must not grow the continuation stack merely to return a tail result. Native C calls must use trampolines without recursive machine re-entry. Sources: (30.08.26, 14), (26.10.26, 3); artifacts/20260529-103914/TASK.md, historical native-call constraint. This bounds control growth, not total heap use.

## Earlier requirements requiring reconciliation with the new substrate

- H1 [historical decision, user]: abstractions remain recognizable runtime values with a binding interface, body, and captured lexical dependencies. Execution lowering must not erase their canonical intensional identity. Source: (29.07.26, 0), artifacts/20260717-120000/basis-experiment.md. Exact current observable representation is not selected. Original parameter spelling need not be part of it.
- H2 [historical decision, user]: only binder values carry environments; ordinary escaping fragments are pure trees. Sources: (30.08.26, 9-11), artifacts/20241225-140425/binders.md. Reconcile with substitution semantics rather than automatically restoring TERM wrappers or nameful lookup.
- H3 [historical decision, user]: closure piths expose declarators and body syntax without an environment slot; rebuilding across a binder boundary rebinds at the destination. Sources: (30.08.26, 9), artifacts/20241225-140425/binders.md. Current substitution/provenance direction and leaf/fork/fruit representations require explicit reconsideration before adopting this exact projection or rebind rule.
- H4 [historical direction, user]: closures and partial applications can be stored and escape, retaining their intended captured dependencies. Sources: (29.07.26, 0-2), (25.09.26, 1), and the current nested-unary closure decision. Exact observation of a partial closure remains open.
- H5 [historical decision, user]: no primitive GLOBAL; external dependencies enter by application, with a module viewed as a function of imports. Source: (09.07.26, 6). This constrains any proposal to use an ambient global binding table.

## Candidate performance criteria, not approved requirements

- P2 [proposal, agent]: reuse unchanged body/code structure rather than traverse and copy the entire body on each call. A February proposal already preferred header copying and parameter indirection. Sources: (01.02.26, 3), July replacement of close-copy by TERM. Neither old implementation is selected for the current design.
- P3 [proposal, earlier agent research]: collect pending arguments, consume available parameters, retain excess arguments, and materialize partial closures only when arguments run out. Source: artifacts/20260613-120000/v0_basis_review.md, Application Semantics.
- P4 [proposal, agent]: performance evaluation should distinguish complete application, escaping partial application, resuming a partial closure, overapplication, nested capture, tree production/inspection, and tail-recursive calls. No benchmark or prototype was run in this requirements pass.
- P5 [proposal, agent]: optimize only across execution boundaries permitted by staging semantics. A returned application tree is not automatically an executable call chain.

## Open questions recovered from earlier work

1. What must inspection reveal: body, remaining inputs, supplied values, captures, binding provenance? Must reconstruction preserve application behavior, inspection, sharing, or some subset?
2. Does applying a closure return a substituted body or enter that body? October records only a tentative preference for returning the body; November selects explicit staging examples without completing a general return protocol.
3. What is the exact contract for excess arguments when the substituted result is itself a closure or a represented application? Do not import the old GRAB body-entry rule without resolving question 2.
4. What does a partial closure expose, and how does resumption preserve lexical dependencies and reusable originals?
5. Is there a zero-parameter closure? Older empty-parameter saturation rules conflict with the historical thunk example. November's unused-parameter wrapper avoids that particular conflict without settling zero arity.
6. Are declarators only syntactic identifiers or runtime-generated terms? identifier-context.md explores dynamic declarators as a working model, not an additional approved requirement.
7. What are the exact shadowing and binding-boundary rules during substitution and generated-tree insertion? Mechanism and observation remain to be selected.
8. Which error cases belong to closure construction/application: invalid declarators, malformed closure values, an unbound identifier in control, or applying a non-callable result?
9. What are the lifetime requirements for escaping closures and shared captures? Mutable boxes and store design were deferred; do not silently bring them into the initial closure scope.
10. Which observations must fast execution preserve when call chains produce code data or are structurally inspected?

## Superseded assumptions to exclude

- Dynamic lookup through the caller's environment: June draft, retracted in July.
- Mandatory nameful runtime representation and no substitution: July/August account, inconsistent with treating the current substitution-only direction as mere spelling changes.
- Automatic eager closure operands: old fn model, contradicted by October/November raw-input and explicit-forcing rulings.
- Retaining triage-calculus reduction rules 0-2, primitive stems, or their old opcode projections: replaced by the lambda core and revised value substrate.
- A mandatory BLOCK continuation implementation: reopened in October while the block result policy remains selected.
- Old letrec/simultaneous-knot requirements: do not import them into today's sequential non-letrec binder decision.
- The ZINC/GRAB mechanism as an already settled complete machine: the current log explicitly classifies the June review as candidate research. Borrowed performance goals do not approve its eager execution policy, code format, or register inventory.

## Sources consulted

The active devlog contains the chronological record and no rotation files currently exist. Relevant current docs were searched. Supporting records consulted:

- artifacts/20260613-120000/v0_basis_review.md
- artifacts/20260613-120000/minimal-machine.md
- artifacts/20260613-120000/v0_cesk_draft.md
- artifacts/20260711-120000/2026-07-11.md
- artifacts/20260717-120000/basis-experiment.md
- artifacts/20241225-140425/binders.md
- artifacts/20241225-140425/identifier-context.md (previous identifier-history pass)
- artifacts/20241225-140425/TASK.md (historical task metadata, not active workflow)
- artifacts/20260529-103914/TASK.md (historical task metadata, not active workflow)
- artifacts/20260529-103914/2026-10-23-anf.md (previous identifier-history pass)
- artifacts/20260529-103914/2026-10-26.md (previous identifier-history pass)
- artifacts/20260529-103915/2026-11-05.md (previous identifier-history pass)
- artifacts/20261020-000000/records.md (targeted ZINC, trampoline, and closure queries)
- artifacts/20261110-000001/new_spec/02_syntax.md and 03_v0.md (targeted searches of retained snapshots)

This ledger is a requirements inventory, not an approved design, execution evidence, or replacement specification. No evaluator code, performance measurements, or external ZINC sources were verified in this pass.
