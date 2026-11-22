# Design session 20261122-095953

Participants: user (research, decisions), agent (proposals, critique, recording).
Starting context: devlog through (21.11.26, 3); open front is execution/identifier identity, see (20.11.26, 8) and (21.11.26, 0..3).

Skills: memory, dialogue, fpf loaded from UberDever/agents@e409100 (linked into ~/.claude/skills).
Ledger lives here, not ./dialogue/, per AGENTS.md artifact placement.

## 0. Framing

Depth: user selects a light dialogue with the user; the agent runs the full colloquium on its own and records it here.
Question: pending (user's last message was cut off after "a policy that is").

Agent-raised candidate topic (not yet accepted by user):
- [assumption] (21.11.26, 0) identity lookup needs stable private bytes; (19.11.26, 14) says private bytes may change on any action. Conflict unless identity forks are exempt.
- [fact] users can construct `~[{:private}, ...]` (19.11.26, 13/14); detection of reliance is optional (19.11.26, 12). So unforgeability is policy, not enforcement.

## 1. Model
## 2. Assumptions register
## 3. Design space
## 4. Candidates
## 5. Objections
## 6. Cruxes
## 7. Decision
## Transcript notes

- Agent objected: identity lookup needs stable private bytes; hand-built identities can collide.
- User: "unstable" means user programs get no guarantee on these bytes when they use them directly. Forging is the same: a program can coincide with the current implementation, but that is unreliable and nasty; this is a policy. Agent concedes both objections (refuted by clarifying scope: user contract vs implementation). Logged as (22.11.26, 0).
- Agent: per-identifier issued identities in lowered trees collide with "no persistence beyond RAM"; offered (a) never persist, (b) renumber on load.
- User: no problem; lowered trees persist as-is and reload under the same language version. Policy is just "user can't do anything with private state; if they do, they are bad". Logged as (22.11.26, 1).
- Agent objection (standing, raised next): two trees persisted from different runs and loaded into one run can carry equal identities issued independently, with no user misuse. Same version does not prevent it; the issuing scheme must be unique across runs.
- User: cross-run collision is valid but implementation-wise; concept unaffected; defer. Logged (22.11.26, 2).

### Topic 2: merge triage into lambda (light dialogue; agent runs full colloquium below)

User proposal, logged (22.11.26, 3):

    [[{:lambda}...], [{x}, {y}, {z}...], [{:cases}, <statements>, {:fail}, <statements>], [{:as-is}, <statements>], ...]

Agent reading (confidence in brackets):
- [high] `{:cases}` group = three positional arms leaf, fork, fruit; an arm is a statement list or the fruit `{:fail}`. Example: fork fails.
- [high] `{:as-is}` group = no shape inspection; operand bound whole.
- [low] how groups map to parameters: one group per parameter in order? alternatives in order? where fork children get names?
- [medium] `<statements>` = new core sequencing, value of last statement?

Objections (status after this turn: all standing, pending user reading):
- O1 child names: fork needs two names, fruit one, leaf zero; param list is the only name source. Conflating arity with destructuring width.
- O2 as-is redundancy: if cases also binds the whole operand and shape inspection never forces, as-is == cases with three identical arms. Needed only if some value is not shape-observable (intrinsics failed in the spike, (20.11.26, 7)).
- O3 statements vs binder: a statement list is a second sequencing mechanism beside `=[..] do end`; ANF already sequences via binders.
- O4 fail semantics: "signaled" (R7RS raise, catchable, needs handler frames in K) vs "is an error"/termination (abort machine). Different machine costs; pick one.
- O5 scope creep: `...` invites more match modes; each mode is a mini-language in the core.
- Antithesis A1: keep triage separate; lambda stays as-is only; fewer cross-cutting rules per form.
Gains noted: one fewer control kind; explicit partiality via `{:fail}` instead of totality-by-construction; lambda/triage share application, environment, and partial-application machinery already (20.11.26, 3).

#### Round 2 — user revision, logged (22.11.26, 4)

    [[{:lambda}...], <private-state>, [{x}, <statements>] [~[<lhs>, <rhs>], <leaf>, <fork>, <fruit>], ...]
    ;; <rhs> could be absent ~; if fork case has no <rhs> it is an error
    ;; {:!} so fruit is inplace; "it is an error"; no raise
    ;; maybe single-param lambdas that nest:
    [[{:lambda}...], <private-state>, [{x}, <statements>], [[{:lambda}...], <private-state>, [~[<lhs>, <rhs>], <leaf>, <fork>, <fruit>], ...]]

Answers to round 1: O4 resolved (no raise, "it is an error"). O1 resolved by clause head carrying names. O5 narrowed: clause kind is the head's shape, not a tag.

New findings:
- [fact] `{:!}` is 2 bytes; w=2 in-place fruit holds 1 byte (layout (19.11.26, 4)). Known-fruit ref or `{!}`. Logged (22.11.26, 5).
- O6 head discrimination vs identities: (21.11.26, 0) gives occurrences identity; declarations must carry the same identity for lookup to match. If a declaration lowers to the identifier tree `[[{:id}], {x}, ~[..]]`, it is a fork, as is `~[lhs, rhs]`; dispatch then needs a tag check at depth 2, not a shape check. [assumption: declarations carry identity in the tree; lambda-lowering artifact (19.11.26, 21) predates (21.11.26, 0) and lowers declarations as bare fruits.]
- O7 clause list is dead past the first clause: both pattern kinds are total, so `...` alternatives never get tried. Unary nesting makes this explicit.
- O8 private-state in unary form: one parameter means no partial-application progress; what does the slot hold?
- O9 rhs absence: make it locally checkable: well-formed only when the fork arm is `{:!}`. Alternative: missing rhs binds lhs to the whole fork, mirroring fruit binding lhs to the whole fruit; then with leaf binding lhs to `~`, `~[x, ~]` subsumes `{x}` (same redundancy as O2).
- Gain of unary nesting: flat/nested distinction (19.11.26, 23) disappears; "at least one parameter" (19.11.26, 22) holds by construction; no runtime cost, since each application already yields a CLOSURE (19.11.26, 19).

#### Round 3

- User: don't commit+push every message. Agent: commit at checkpoints.
- User (1): `:` prefix = symbol, registry, well-known symbols as known fruits; clarity over space. Logged (22.11.26, 6). Fail-marker width concern withdrawn.
- User (2): head is the fruit `{x}`, not an identifier. Agent misread: O6 rests on an inference, not on the text. Restated O6: under (21.11.26, 0) the declaration's identity must be stored somewhere; a bare-fruit head has no room. Candidate: the lambda's `<private-state>` slot carries the declaration identities.
- User (3): agent misread; N clauses = N-ary lambda. O7 refuted. Unary nesting withdrawn by user: better sharing but materializes lambdas in place, against ZINC; keep multi-param with COW. Logged (22.11.26, 7). O8 moot (private-state still holds progress).
- User (4): unclear. Agent to restate O9 with an example.
- New question Q1: what do statements of clause i < N do? Run when argument i arrives (staged), or only the last clause's body runs?

#### Round 4

- User: no statements, only an expression; agent should have reminded. O3 resolved by user; agent concedes it let O3 drop. Logged (22.11.26, 8).
- User: naive composition of multi-param lambdas with patterns explodes (3^N); how do pattern calculi and pattern-matching languages handle it? Baseline: listed cases only, error on others. Logged (22.11.26, 9).
- Agent survey: `patterns-survey.md`, logged (22.11.26, 10).
- Candidates (palette, no ranking):
  - C1 clause matrix in core: lambda = N params, ordered rows of N patterns plus one body each; first match wins; no match is an error. Tree keeps authored structure.
  - C2 one-level core: keep a unary three-arm shape split (today's triage, or a lambda clause); vf lowers multi-arg and nested patterns to nested splits (decision tree). Core stays small; structure is erased.
  - C3 failure as value: Jay-style `p -> s | r`; each case is a separate unary value; alternatives compose by extension; an unhandled failure at the top is an error.
- Crux K1: must compile-time code see the authored clause matrix in v0 (favors C1/C3), or is a vf-level record enough (favors C2)?
- Open: name vs fruit-literal pattern ambiguity: if a head `{x}` is a name, a fruit literal pattern needs a marker.
- User: too rigorous for this stage; write less. Agent: short replies, one point or question at a time.
- User picks C1, shape patterns only; M cases x N params; default whole-tree binding; discriminative patterns, any order; mixing shape and general in one position is an error; binders are identifiers. Logged (22.11.26, 11). K1 resolved toward C1. O6 resolved: binders carry identity.
- Note (user, ledger only): case-lambdas have many corner cases; rigorous specification pending.
- User sketch logged (22.11.26, 12): `[[{:lambda}], <ps>, ~[[~[{:all}, <id>], ~[{:fork}, <id1>, <id2>], ...], expr], ...]`.
- Agent: `~[{:fork}, <id1>, <id2>]` has three elements; a pair holds two. Needs `[{:fork}, id1, id2]` or `~[{:fork}, ~[id1, id2]]`.
- Agent: with order-free cases, after k < N arguments several cases can still match; private state needs the set of surviving cases, not one branch.
- User: fork pattern `~[{:fork}, ~[id1, id2]]`. Logged (22.11.26, 13).
- User (tentative): cases match in order as written; the old triage encoding materialized trees explicitly and avoided the surviving-set issue.
- Agent: order resolves overlap, not timing. Cases [fork, leaf] and [fork, fruit] both survive the first argument either way. Alternative: match only when all N arguments are bound; shape tests do not force, so deferring loses nothing, and private state is again just progress. Not logged yet; awaiting user.
- User: worried about devlog noise. Agent review: 14 entries today; (22.11.26, 5) and (22.11.26, 13) were low value; drafts (3), (4) could have stayed in the ledger. IDs cannot be removed. From now on: devlog gets decisions and surviving questions only; drafts and agent critique stay here.
- User agrees on matching at saturation; "lambda superposition, dead and alive until settled"; CLOSURE installed on the whole lambda. Logged (22.11.26, 14).
- Agent: no real superposition: arguments sit in a positional buffer in private state; nothing binds until the match picks a case.
- User: triage on partial lambdas breaks with per-case names (cannot tell which ids are bound). Moves to a general signature: `[[{:all}, id1], [{:triage}, id1, id2]]`; each body lists expected shapes for the triage params, e.g. `[{:fork}, {:leaf}, {:fruit}]`. Logged (22.11.26, 15), supersedes (12), obsoletes (13).
- Agent: fixed names per position restore per-argument env extension; the positional buffer from round (14) is no longer needed.
- Agent point: no wildcard shape in a case; a body indifferent to one triage position needs 3 duplicate cases. Allowing `{:all}` as a shape wildcard reintroduces overlap, hence case order.
- User: `{:all}` positions are the implicit wildcards; bodies list only triage shapes. (Agent meant a wildcard inside a triage position.)
- User design B: header is a row of bindings (one or two ids per position); each body carries a full row of expected shapes including an explicit wildcard; corner cases are errors.
- Agent: if overlap between cases is an error, B stays order-free and is strictly more expressive than A: [fork, *] and [leaf, fruit] do not overlap. Not logged; awaiting choice between A and B.
- User picks B. Logged (22.11.26, 16). User asks: what is the harm in allowing overlaps?
- User allows overlap. Logged (22.11.26, 17).
- User sketch: `[[{:lambda}, ...], <ps>, [<id1>, <id2>, <id3>, ...], ~[[{:all}, {:fruit}...], expr], ...]` with a flat identifier list.
- Agent: if a position's id count comes from each case's shape, the same id means different things per case, which reopens the partial-triage problem of (22.11.26, 15). If it comes from all cases together, adding a case can shift every later name. Suggest grouped header per position. Not logged.
- User accepts grouped header. Logged (22.11.26, 18). User asks whether discrimination is needed here.
- Agent: group length (one or two ids) discriminates; ids are already tagged trees. Rule: a fork shape on a one-name position is an error.
- User: length is enough. Folded into (22.11.26, 18).
- User: no triage form remains; private state = count. Logged (22.11.26, 19). Spec 02_v0.md still lists Triage and {:triage}; edit only on request.

### Topic 3: vf syntax for case-lambdas

Agent proposal (from snapshot grammar: `\[..]` closure_head, `?` triage, `~[a, b]` nyad):

    \[n, ~[l, r]]
        ?[_, fork] do f(l, r) end
        ?[_, leaf] do n end
        ?[_, _]    do fallback end

- header param: identifier (one name) or `~[l, r]` (two names); `?[...]` row of shapes `leaf`, `fork`, `fruit`, `_` + block.
- `\[x] do x end` unchanged: no rows = one all-wildcard case.
- `?` prefix needed: `[_, fork] do ... end` would parse as a list with a loose block postfix.
- User: `_` is an ordinary identifier; row heads hold symbols; no `do end`; `\[]` is a prefix form; after `?[]` expect one expression; no plain expression once a row is present. Logged (22.11.26, 20).
- Agent: (a) `:fork` already parses as a label-less labeled expression; symbol spelling needed (`{:fork}` works today). (b) ASI ends an expression at newline, so rows on later lines would end the lambda. (c) Nested case-lambdas: trailing rows attach to the innermost lambda (dangling else).
- User: `{:fork}` verbose but this is low-level; innermost attachment with parens; multiline lambda should parse whole. Logged (22.11.26, 21).
- Agent: ASI rule (snapshot l.226-237) ends a complete expression at newline; candidate amendment: a newline followed by `?[` continues the innermost case-lambda (one-token lookahead). Zero-change option: wrap in parens.

### Topic 4: forcing program in case-lambda syntax

- User: no ASI amendment; the devlog will be replayed when the syntax chapter is revised.
- Agent rewrote artifacts/20261120-172943/force.vf as `force.vf` here (not run). Triage applications become applied case-lambdas; tag-of destructures its parameter directly; forcing binders kept as in the original. Every multiline lambda sits inside brackets or parens, so ASI does not interfere.
- Agent: `force-end.vf` applies `end.` (20.11.26, 6) with the reading: close every open block up to the nearest enclosing delimiter, or all at top level. Not run.
- User: binder body is one expression, so `=[...] expr` without `do end`; sequential clauses can hold the work and the body is the result. Agent agrees; `force-flat.vf` (not run) assumes clauses bind in order (let*), as the snapshot example `=[x: .., y: x + ..]` implies, and that each clause forces its value. Not logged; awaiting confirmation.
