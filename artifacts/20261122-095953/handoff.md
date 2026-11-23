# Where we left off (23.11.26)

Read this, then the devlog sections `### 22.11.26` and `### 23.11.26`.

## The language as decided

- **Lambdas** are case-lambdas: `[[{:lambda}], <private state>, <header>, ~[<shape row>, <expr>], ...]`.
  Header positions bind one name or `~[l, r]`; rows use `{:leaf}`, `{:fork}`, `{:fruit}`, `{:all}`.
  A case is selected when all arguments are in, first match wins; triage is gone.
  (22.11.26, 3), (22.11.26, 4)
- **Syntax:** `\[...] expr` or `\[...] ?[...] expr ...`; binders `=[a = e, ...] expr`, clauses in
  order. (22.11.26, 7), (22.11.26, 8)
- **Substitution:** capture-avoiding with private priming, simultaneous per activation, general
  shadowing; a binder runs however it is reached. (23.11.26, 0)
- **Declarations:** a written slot declares its name; computed declarations are built as data.
  (23.11.26, 1)
- **Holes:** bound only when code is assembled around them within one activation. (23.11.26, 0)
- **Unset names:** mentioning one in a selected body is an error. (23.11.26, 3)
- **Partial applications** hide their bound arguments. (23.11.26, 4)
- **Identifiers:** `[[{:id}], <name>, ~[{:private}, <run id + counter> <prime>]]`. (23.11.26, 5)
- **Cells:** kinds `000` leaf, `001` fork, `010` unknown fruit, `011` indirect, `101` known fruit,
  `110` CLOSURE; `100` and `111` free; no ref cycles. (22.11.26, 20)
- **Store:** addresses `[[{:address}], <private>]` map to terms. (22.11.26, 18)
- **Recursion** through `fix`; a whole fork is rebuilt as `~[h, t]`. (22.11.26, 17), (22.11.26, 19)

## Waiting for you

- **Identity table** (proposal, kind `100`): `identity_table.py` has the types and operations.
  (23.11.26, 6)
- **Known-fruit preload** of all one-byte fruits (proposal). (22.11.26, 20)

## Next work

- [ ] How the hand-written environment machine realises holes, capture avoidance and priming;
  cross-test it against `solutions/cas.py`. (23.11.26, 8)
- [ ] Replay these decisions into the spec. (23.11.26, 9)
- [ ] Older open items: binding contract (22.11.26, 9), case dispatch (22.11.26, 10), syntax
  alignment (22.11.26, 11).

## Files (all throwaway)

- `solutions/cas.py` and `solutions/scheme.md`: the current scheme, executable; `run_cas.py` runs
  the 37-case catalogue (all pass).
- `solutions/comparison.md`: the roles / provenance / fresh-identity comparison.
- `reference-machine/`: literal rewriting machine. `case-lambdas/`: the first evaluator, the
  v0 compiler (`compile_v0.py`), `fix`, factorial, Fibonacci.
- `substitution-machines.md`: CESK, ANF and explicit-substitution notes. `dialogue.md`: the
  session record.
