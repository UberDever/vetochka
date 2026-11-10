# Lexical reference models for v0 CESK

Status: accepted Model B. 2026-07-11.

## Settled common substrate

- Raw normalized syntax is inert data. A forged identifier is `[{:id}, {name-bytes}]`.
- `$f` / `$fn` create lexical closures. Eager call operands are evaluated in caller
  context; closure bodies run in defining capture extended by call bindings.
- `TERM(root; private E)` is a real runtime family, like `OPCODE`. It objectively
  associates an open term with lexical environment; `E` is opaque and unforgeable.
- CESK may thread E in control without allocating TERM on every descent, but must
  preserve `(root,E)` whenever open code returns, is structurally stored, becomes a
  pending argument, or crosses a continuation/frame.
- Lists are inert proper L0 structures. They do not evaluate elements.
- A TERM structural view is transparent to triage: a selected raw child retains its
  TERM environment; a child already carrying TERM retains its own environment.
- `TERM(APPLY(f,x),E)` evaluates callee under E and puts `TERM(x,E)` on pending
  argument spine. Delta stores pending TERM without forcing it. Rule 1 returns the
  selected callee branch; Rule 2 gives x/y callee E and duplicated z caller E; Rule
  3 gives selected argument branches argument E.
- `TERM(TERM(root,Einner),Eouter) = TERM(root,Einner)`. Lexical frame extension
  occurs only when a closure is applied, never at TERM collapse.
- v0 is effectful: source/transition order defines effects and hard traps.

## Model A: resolved LOCAL executable IR

### Construction

When `$f`/`$fn` receives a body:

1. Obtain resolver scope S that maps source names to lexical addresses.
2. Extend S with parameter binder(s).
3. Traverse body syntax, replacing identifier syntax with `LOCAL(address)`.
4. Do not traverse nested `$f`/`$fn` bodies; each is resolved later with its own
   binders.
5. Closure retains resolved body plus opaque defining E.

`LOCAL` is a real executable-cell family, never source syntax.

### Execution

```
TERM(LOCAL(n), E) -> E[n]
```

A function call makes `Ebody = parameter-frame :: Edef`, then executes
`TERM(resolved-body,Ebody)`. TERM preserves E through inert delta/list structure,
so `^ LOCAL(0)` does not escape as a bare dead local.

### Required hidden metadata

A nested raw function body must later resolve outer source names. Its current E must
therefore carry private binding-name/layout metadata, or TERM/closure must also carry
a private resolver scope S. This is the unresolved seam in Model A.

### Properties

- Executable cells are name-free and alpha-invariant.
- Slot lookup can be compact.
- Unknown names can be rejected while resolving a body.
- Requires body traversal and resolver scope/layout machinery.
- TERM remains necessary for escaped open structure.

## Model B: nameful TERM code

### Construction

No source-name lowering pass. Function closure stores raw normalized body, binder
name(s), and opaque persistent defining E:

```
closure = (body-syntax, binder names, Edef)
```

At call, eagerly evaluate argument(s) in caller TERM context, then create:

```
Ebody = extend(Edef, binder-name -> value)
TERM(body-syntax, Ebody)
```

Nested function construction simply captures its current E; no separate S exists.

### Execution

```
TERM([{:id},{name}], E) -> lookup(E,name)
```

Name lookup happens only on executable demand. A literal byte value `{name}` is not
an identifier and never looks up. Raw forged identifier syntax becomes lexical code
only by being paired with a particular E.

### Properties

- One persistent lexical object E; no LOCAL node, de Bruijn shifting, or resolver
  traversal.
- Natural for code forging: code can capture current E but cannot forge another E.
- Byte-name lookup occurs on each demanded identifier.
- Unknown names hard-trap on demand, like Python/JavaScript name lookup; effects
  earlier in source order may already have occurred.
- Alpha-equivalent executable terms differ structurally.
- Mixed-scope rewrites must retain TERM boundaries. This is also required by Model A.
- VM must intercept TERM before pure reducer triage sees a raw identifier. This is
  also required by Model A for LOCAL.

## The important non-difference

Forged names do not by themselves create dynamic scope. Dynamic scope occurs only
if a term is evaluated in caller E rather than its captured TERM E. Both models avoid
that when TERM association is objective.

## Standard ML comparison

The Standard ML Definition specifies nameful dynamic environments. Its function
closure is `(match,E,VE)`, where E is defining environment and VE supports recursive
bindings. Identifier lookup uses E by name; function application evaluates a match
under captured E extended by recursive VE. It does not prescribe compiled local-slot
representation. Citations: *The Definition of Standard ML (Revised)* (1997), §6.3
Figure 13 p.44, §6.6 pp.45-46, §6.7 rules 91/102/108 pp.47-49, rule 126 p.52.

Thus Model B resembles SML semantic environments. Model A is a compiler/IR choice
that may implement the same lexical semantics.

## Decision

Selected: **Model B — nameful code plus TERM and persistent nameful E**.

- Identifier syntax remains nameful executable code; `TERM([{:id},{name}],E)`
  resolves `{name}` only on executable demand through its captured E.
- Function closure stores raw body, binder names, and opaque defining E; call extends
  E with eager argument values.
- `LOCAL` is not v0 semantic IR or cell ABI. A private resolved-slot cache may later
  optimize execution if measurement justifies it, but must preserve Model B behavior.
- Rejected for v0 now: Model A resolved LOCAL IR. It adds resolver traversal, separate
  scope/layout machinery, and LOCAL ABI without demonstrated need.

This choice changes L3, cell ABI, diagnostics timing, and code-forging ergonomics.
It does not change L0 triage or TERM environment propagation.
