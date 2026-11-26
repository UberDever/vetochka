# Disp: notes from source

Disp (https://github.com/libdither/disp, site disp-lang.net) is another language built on tree calculus.
Read on 26.11.26 at commit `0227c231f084af1bbcb6e04d0bca209a91d06232` (2026-11-26).
disp-lang.net itself was not reachable from the session. Paths below are relative to that repository.

## Assessment

User: Disp is very predictable, the logical thing to do with triage calculus in the first place.
Nothing in its runtime was judged interesting for Vetochka.

## Runtime

`src/core/tree.ts` holds one `apply` loop. Its cases are Jay's rules unchanged:
leaf and stem construction, K, S, and triage on leaf, stem and fork. Around them:

- Hash-consing. Leaf, stem and fork nodes are interned in a global arena sharded 64 ways, so structural equality is node identity.
  Scopes un-intern unreachable nodes. There is no other reclamation.
- Memoized application per `(f.id, x.id)`, up to 5M entries. K results are not memoized.
- S shortcuts. K-discard: if `c x` is `K w`, the result is `w` and `b x` is never evaluated.
  C fast-path: if `b` is `K u`, the result is `c x u`. The code notes that termination depends on the discard.
- `susp(f, a)`, a fourth node kind: an unevaluated application, created only for a native applied to its first argument.
  Applied to a second argument, the host handler answers. Observed structurally (triage, comparison, printing, operator position), it is forced to the reduct of the in-language definition, memoized on the node.
  A handler returning `null` falls back to the in-language definition.
  Reason: application is unary, so an arity-2 native needs a partial value, and because triage can inspect that partial, it must equal the genuine reduct.
- Exactly two natives, registered by name at their definition site (`src/elab/driver.ts:231`):
  - `tree_eq` (`lib/prelude.disp`): host identity check, returning `true = △` or `false = △△`.
  - `m_advance` (`lib/machine.disp`): the eager evaluator loop as a tree value, with states `Done v`, `Run (f, x) stack` and `Held`, and frames `To`, `Res` and `S`; `m_advance m k` runs k steps.
    `stepMachine` reimplements it in the host.
    Library uses: step-bounded runs for cost claims (`lib/theories/cost.test.disp`), time-sliced streams and tasks (`lib/stream.disp`), and symbolic evaluation that pauses on applications reading hypotheses (`lib/confirm.disp`).

Evaluation is eager. Other backends (Rust ports and an interaction-net reducer without hash-consing) are tested differentially against the TypeScript eager reducer, which is the oracle.

## Language

- Surface code is compiled to trees by bracket abstraction. Reflection inspects the resulting combinator trees, not source structure.
- Data are pure tree encodings: `zero = t`, `succ n = t(t, n)`, strings are lists of codepoint naturals.
- Types are recognizer programs. The checker (`lib/kernel/kernel.disp`) is a Disp program. Effects are data interpreted at the boundary.
- Stated goal: a self-verified dependently typed language whose optimizer, written in the calculus, synthesizes the fastest program for a specification. The optimizer is not built.
- Implementation: TypeScript host, Rust backends. Most code is reported as written with Claude.

## Contrast with Vetochka

The contrast is with the state of the log up to (24.11.26).

- Vetochka keeps tree-shaped data and shape dispatch, but executes on a CESK machine with case-lambdas, environments and intrinsics, not on Jay's rules.
- Vetochka intrinsics take argument lists (12.11.26, 11) and have no in-language definition to match, so neither reason for `susp` applies.
- Spans in tags and parser-issued binding identities limit the structural sharing that hash-consing exploits in Disp, and effectful intrinsics make memoized application unsound in general.
