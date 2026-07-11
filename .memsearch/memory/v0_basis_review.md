# v0 Calculus Basis and Machine Review

Status: current v0 decision summary. Detailed rules live in
[`v0_cesk_draft.md`](v0_cesk_draft.md).

## Constraints

v0 must be:

- small enough to implement and bootstrap;
- programmable directly in v0 syntax;
- suitable for lexical functions and compiler code;
- compatible with triage leaf/stem/fork values and reflection;
- able to call strict, non-strict, and native operations;
- inspectable and debuggable before optimization.

## Calculus Bases

| Basis | Useful part | Main cost | v0 verdict |
|---|---|---|---|
| Generic term rewriting | Natural compiler and syntax transforms | Naive redex search costs roughly rules x nodes per step; compiled dispatch becomes another VM | Keep as source-transform model, not runtime basis |
| Concatenative/Forth | Tiny stack machine, simple composition | Lexical capture, tree reflection, and bootstrap source become awkward | Reject for v0 |
| Logic/relational | Good for resolution and constraint problems | Search, backtracking, effects, and evaluation order complicate runtime | Possible library or compiler tool |
| Dataflow | Good dependency and incremental-build model | Poor general bootstrap and metaprogramming basis | Possible subsystem |
| Actors/process calculus | Explicit concurrency and isolation | Solves concurrency, not current compiler/runtime problem | Defer |
| Effects/continuations | Explicit control and host interaction | Not a complete ergonomic programming basis by itself | Add as explicit operations when needed |
| Interaction combinators/nets | Local active-pair rewrites; no global redex search | Requires arbitrary graph wiring, sharing, graph GC/debugging, and effect-order policy | Research/backend option, not v0 |
| Pure combinators/triage only | Minimal calculus, maximal structural reflection | Raw abstraction and bootstrap code are too hard to write | Keep triage as value/reduction layer, not sole authoring basis |
| Lambda/application | Familiar functions, lexical scope, practical bootstrap | Needs closures, environments, and evaluation-mode rules | **Selected** |

Lambda is selected from implementation fit, not theoretical uniqueness. Triage
remains part of runtime semantics: lambda machinery does not replace delta values
or their structural reduction.

## Abstract Machines

| Machine | Strength | Cost or mismatch | Verdict |
|---|---|---|---|
| CEK | Small, clear lexical call-by-value semantics | No semantic store for shared mutable boxes | Immutable implementation subset |
| CESK | CEK plus explicit store for recursion, mutation, and boxes | Store adds allocation and indirection | **Selected and extended** |
| Krivine | Small call-by-name machine; natural suspended terms | Repeats work and effects; weak-head evaluation only | Reject as default |
| Lazy Krivine/STG | Sharing and call-by-need | Heap update, black holes, GC, and larger runtime | Defer |
| SECD | Historical stack/environment/control machine | Dump and fixed evaluation structure add little here | Reject |
| ZINC/ZAM | Curried calls without intermediate closures when enough arguments exist; argument spine and tail calls | Stock machine assumes uniform call-by-value and argument-first compilation | Borrow `GRAB` calling convention |

Selected split:

- extended CESK defines v0 semantics;
- ZINC-derived argument spine and `GRAB` optimize closure calls;
- triage and opcode dispatch extend callable kinds.

## Selected v0 Model

Pipeline:

```text
source/CST
  -> nameful syntax data
  -> resolved specialized v0 IR
  -> cells
  -> CESK execution
```

Specialized IR contains executable distinctions such as `APPLY`, lexical/global
references, literals, delta, and native opcodes. Cells are compact storage for this
IR and ground term data. Machine stacks, environments, closures, and suspended
terms may use separate runtime objects.

Machine state conceptually contains:

```text
code/control + environment + store + argument spine + return stack + current value
```

This is extended CESK with a ZINC-derived closure calling convention. Cells may
implement store locations, but immutable code/data cells and mutable box slots have
different semantic roles.

## Application Semantics

Generic application evaluates callee first. Callee then selects argument mode:

- `{fn}` evaluates argument to weak-head normal form, then binds value;
- `{form}` binds `TERM(code, environment)` without evaluating argument;
- delta receives terms according to triage rules;
- native opcode declares its argument policy.

Closures capture lexical creation environment. `TERM` captures code plus current
environment and is runtime suspension, not raw source syntax.

Applicable families retain homogeneous saturation:

```text
TYPE0 -> TYPE1(x) -> TYPE2(x, y) -> family dispatch
```

Each arrow consumes one argument. This protocol applies to delta/opcode node
families, not arbitrary-arity closures. Closures consume pending arguments with a
ZINC-style `GRAB` loop.

Delta, opcode, and runtime-wrapper families may remain outer-shape inspectable.
`APPLY(code, argument)` is different: pending execution node, not saturated value.

Weak-head evaluation may execute multiple head `APPLY` nodes before a runtime value
appears. It does not recursively normalize value children. Executing exactly one
`APPLY` would be a separate step operation.

Application spine collection pushes argument `TERM`s without evaluating them.
`GRAB` then:

- binds all available parameters, forcing only eager ones;
- enters body directly for exact application;
- leaves excess arguments for overapplication;
- allocates one partial closure only for underapplication.

## Immediate Caveats

- Non-strict `TERM` is call-by-name; duplicate forcing deliberately repeats effects.
- CESK must own application dispatch; generic cell arity cannot determine semantics.
- Runtime `TERM` and source-syntax handles must remain distinct.
- Source-tree handle and specializer API details remain implementation work, not
  unresolved calculus semantics.
- Seed compiler must only recognize v0 and resolve names into specialized IR.

Deferred until required by working programs:

- call-by-need and memoized thunks;
- exact box layout and transparent mutable-binding lowering;
- runtime-object serialization and equality;
- environment inspection;
- linear bytecode, threaded dispatch, and production GC.

## Decision

Use lambda/application as v0 programming basis, triage as structural
calculus/value layer, extended CESK as execution machine, and a ZINC-derived `GRAB`
calling convention. Immutable execution is its CEK subset; boxes activate store.
