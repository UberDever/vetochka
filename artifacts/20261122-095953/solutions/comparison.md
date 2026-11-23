# Declarations versus usages: three solutions compared

Question (22.11.26, 29): after rewriting, a nested lambda written in the code and one inserted as
a value look the same, so a rule that only looks at the tree cannot tell a computed declaration
(`mk`) from a real one (compose). The user proposed three ways out. Each is implemented as a
variant of the reference machine in `variants.py`, and the catalogue in `catalogue.py` runs on
all of them (`python3 run_catalogue.py`, about 10 s, writes `results.md`).

## The variants

| variant | what declares | what is rewritten | shadowing |
|---|---|---|---|
| base | an identifier in a declaration slot | identifiers | general (22.11.26, 26) |
| roles (1) | `~[{:decl}, ID]` in a slot | `~[{:use}, ID]` only; a plain `ID` is inert | by `{:decl}` |
| provenance (2) | an identifier in a slot | identifiers; every inserted fork is sealed | only at seals: a seal protects what lambdas and binders inside it declare; written code has none |
| fresh (3) | an identifier in a slot | identifiers | none; a lambda's first application and a binder's start give every declaration in its tree a fresh identity |
| fresh+shadow | as fresh | as fresh | general |

A declaration slot is a name in a lambda header position or the left side of a binder clause.
Programs are written once with `%x` ("declare what `x` holds") and `^x` ("use what `x` holds"),
then run in three forms: plain (`$x`, the counting rule), role (`~[{:decl}, $x]`,
`~[{:use}, $x]`), and data (today's idiom: header or clause list built as data first).

## Results

✓ means the intended result. Full table with values: `results.md`.

| case | base | roles | provenance | fresh | fresh+shadow |
|---|---|---|---|---|---|
| A1–A5 lexical basics | ✓ | ✓ | ✓ | ✓ | ✓ |
| B1–B6 inserted values: compose, twice, `fix`, self-application, partial holding a lambda | ✓ | ✓ | ✓ | ✓ | ✓ |
| C1, C4 hole as data; a different hole escapes | ✓ | ✓ | ✓ | ✓ | ✓ |
| C2, C3, C6 binder built around a hole binds it (plain form) | ✗ | ✗ | ✓ | ✓ | ✗ |
| same, role form | — | ✓ | — | — | — |
| same, data form | ✓ | ✗ | ✓ | ✓ | ✓ |
| C5 running a hole is an error | ✓ | role form only | ✓ | ✓ | ✓ |
| D1 `mk`, D2 computed clause declaration (plain form) | ✗ | ✗ | ✓ | ✓ | ✗ |
| same, role form | — | ✓ | — | — | — |
| same, data form | ✓ | ✗ | ✓ | ✓ | ✓ |
| D3 reflective re-entry | ✓ | ✓ | ✓ | ✓ | ✓ |
| D4 two nested declarations of one generated identity | `{2}` (shadows) | `{2}` | error | error | `{2}` |
| E2 take a lambda apart and rebuild it | ✓ | ✓ | ✓ | ✓ | ✓ |
| F1 an arm mentions its unset name | ✓ | ✓ | ✓ | ✓ | ✓ |
| F2 unset name fed back into the same lambda | ✗ captured | ✗ captured | ✗ captured | ✓ | ✓ |
| G1–G4 closures, mutual recursion, depth 300, factorial and Fibonacci | ✓ | ✓ | ✓ | ✓ | ✓ |
| H1–H3 store | ✓ | ✓ | ✓ | ✓ | ✓ |
| I1 persist `g(g(id))` as plain text, reload, apply | ✓ | ✓ | ✗ | ✓ | ✓ |

"—" marks a form written for another variant: the role form is malformed outside roles.

What every variant shares (H2): a binder binds only the holes present in its text when the clause
binds. A hole that arrives later, from the store or from a call, stays a hole. That is what
rewriting in place does, and no variant changes it.

## Per solution

### 1. Explicit roles

- Works whenever the program states roles: computed declarations, compose, `fix`, holes.
- A plain `ID` is inert: rewriting never touches it, and running it returns it as data (C5 plain).
  Names can therefore be carried inside the scope that binds them without being captured. This is
  quoting, which (22.11.26, 13) ruled out for bound identifiers ("a bound identifier is its
  tree"); roles reopen it on purpose.
- Every generated name needs the right role. Forgetting one fails silently (C2 plain returns the
  hole), and the data idiom fails loudly (a plain `ID` in a slot is not a declaration).
- F2 still captures: an unset name stays a `{:use}`. A small change would close it: leave unset
  names as plain `ID`s, which are inert (not run).
- Shadowing stays, keyed by `{:decl}`; no renaming.
- Cost (argued): one extra fork per identifier occurrence in lowered code, 4 bytes with two-byte
  refs; a tag check to recognise usages. No per-activation work.

### 2. Provenance

- Works with no markers: the plain form of every computed declaration, plus compose and `fix`,
  because inserted values are sealed and protect their own declarations.
- F2 still captures: a seal protects only identities declared inside it, and an unset name is
  declared by the lambda receiving it.
- Persistence needs a text form that shows seals. Plain text drops them, and the reloaded compose
  result then rewrites its inner header (I1). The form must be one that programs cannot build,
  or rebuilding a tree could forge provenance. `display_sealed` prints seals as `#(...)`, which
  is new notation.
- Seals are invisible to inspection and equality, yet two equal trees can behave differently
  under later rewriting: one assembled from inserted parts, one written out.
- D4: a nested repeat in written code is a usage, so the inner declaration is rewritten (error).
- Cost (argued): a seal node per inserted fork value, such as a CLOSURE-like ref (kind `111` is
  free); the set of declarations inside each seal, cached per seal. A deferred machine has this
  structure already: inserted values are closures with their own environment.

### 3. Fresh identities on activation

- Passes everything with an intended result, in the plain and data forms: computed declarations
  without markers, compose, `fix`, holes, F2, and plain-text persistence.
- The renaming has to be deep: activating a lambda also renames the declarations of nested
  lambdas and binders in its tree. Renaming only the lambda's own parameters fails compose, as
  argued in (22.11.26, 29): the inserted inner lambda would share its parameter with the outer
  one. Within one declaration's scope a repeat is a usage, so D4 is an error.
- Identity bytes change on every activation (E1 shows `~{02}` becoming `~{00 00 80 04}`).
  Spellings stay; the bytes were never guaranteed (22.11.26, 0).
- Fresh identities are consumed fast: a two-byte counter ran out within this catalogue, so
  identities need at least four bytes, or reuse. This adds pressure on cross-run identity
  collisions (22.11.26, 1): a persisted tree can hold holes whose identities a new run will issue
  again. I1 passes because every declaration is renamed again on activation, but a hole is not.
- Adding shadowing back (fresh+shadow) brings the plain-form failures back. Renaming already
  prevents every collision that shadowing guarded against, and shadowing blocks computed
  declarations.
- Cost (argued): in the literal machine, one renaming walk per activation, comparable to the copy
  it makes anyway. In a deferred machine, a lambda's own parameters are fresh per activation for
  free (each activation has its own bindings, as CESK allocates fresh addresses); nested
  declarations get fresh bindings when they activate, and closures keep inserted values apart,
  so the deep renaming of the literal machine has no runtime counterpart.

## Comparison

| | roles | provenance | fresh |
|---|---|---|---|
| computed declarations | explicit `{:decl}`/`{:use}` | written directly | written directly |
| programmer burden | every generated name carries a role | none | none |
| F2 (own name fed back) | captured, fixable by making unset names inert | captured | correct |
| plain-text persistence | yes | needs a seal syntax | yes |
| equal trees interchangeable | yes | no | yes, up to identity bytes |
| quoting names | possible (plain `ID`) | no | no |
| shadowing needed | yes | at seals | no |
| identity bytes | stable | stable | fresh per activation |
| runtime work | none extra | seal per insertion | rename per activation (literal); free in a deferred machine |

## Recommendation (agent)

Fresh identities on activation. It is the only variant that is correct on every case with an
intended result, it is your counting rule made safe, persistence stays plain text, and in the
deferred machine it is what per-activation bindings already give. Its costs are identity bytes
that change and the need for wider, run-unique identities (22.11.26, 1). Roles are the choice if
explicit quoting of names is wanted, with unset names made inert. Provenance moves complexity
into persistence and breaks tree interchangeability for no case that fresh identities miss.
