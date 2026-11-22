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
- User: no problem; lowered trees persist as-is and reload under the same language version. Policy is just "user can't do anything with private state; if they do, they are bad". Logged as (22.11.26, 0).
- Agent objection (standing, raised next): two trees persisted from different runs and loaded into one run can carry equal identities issued independently, with no user misuse. Same version does not prevent it; the issuing scheme must be unique across runs.
- User: cross-run collision is valid but implementation-wise; concept unaffected; defer. Logged (22.11.26, 1).

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

#### Round 2 — user revision, logged (22.11.26, 3)

    [[{:lambda}...], <private-state>, [{x}, <statements>] [~[<lhs>, <rhs>], <leaf>, <fork>, <fruit>], ...]
    ;; <rhs> could be absent ~; if fork case has no <rhs> it is an error
    ;; {:!} so fruit is inplace; "it is an error"; no raise
    ;; maybe single-param lambdas that nest:
    [[{:lambda}...], <private-state>, [{x}, <statements>], [[{:lambda}...], <private-state>, [~[<lhs>, <rhs>], <leaf>, <fork>, <fruit>], ...]]

Answers to round 1: O4 resolved (no raise, "it is an error"). O1 resolved by clause head carrying names. O5 narrowed: clause kind is the head's shape, not a tag.

New findings:
- [fact] `{:!}` is 2 bytes; w=2 in-place fruit holds 1 byte (layout (19.11.26, 4)). Known-fruit ref or `{!}`. Logged (22.11.26, 2).
- O6 head discrimination vs identities: (21.11.26, 0) gives occurrences identity; declarations must carry the same identity for lookup to match. If a declaration lowers to the identifier tree `[[{:id}], {x}, ~[..]]`, it is a fork, as is `~[lhs, rhs]`; dispatch then needs a tag check at depth 2, not a shape check. [assumption: declarations carry identity in the tree; lambda-lowering artifact (19.11.26, 21) predates (21.11.26, 0) and lowers declarations as bare fruits.]
- O7 clause list is dead past the first clause: both pattern kinds are total, so `...` alternatives never get tried. Unary nesting makes this explicit.
- O8 private-state in unary form: one parameter means no partial-application progress; what does the slot hold?
- O9 rhs absence: make it locally checkable: well-formed only when the fork arm is `{:!}`. Alternative: missing rhs binds lhs to the whole fork, mirroring fruit binding lhs to the whole fruit; then with leaf binding lhs to `~`, `~[x, ~]` subsumes `{x}` (same redundancy as O2).
- Gain of unary nesting: flat/nested distinction (19.11.26, 23) disappears; "at least one parameter" (19.11.26, 22) holds by construction; no runtime cost, since each application already yields a CLOSURE (19.11.26, 19).

#### Round 3

- User: don't commit+push every message. Agent: commit at checkpoints.
- User (1): `:` prefix = symbol, registry, well-known symbols as known fruits; clarity over space. Logged (22.11.26, 2). Fail-marker width concern withdrawn.
- User (2): head is the fruit `{x}`, not an identifier. Agent misread: O6 rests on an inference, not on the text. Restated O6: under (21.11.26, 0) the declaration's identity must be stored somewhere; a bare-fruit head has no room. Candidate: the lambda's `<private-state>` slot carries the declaration identities.
- User (3): agent misread; N clauses = N-ary lambda. O7 refuted. Unary nesting withdrawn by user: better sharing but materializes lambdas in place, against ZINC; keep multi-param with COW. Logged (22.11.26, 5). O8 moot (private-state still holds progress).
- User (4): unclear. Agent to restate O9 with an example.
- New question Q1: what do statements of clause i < N do? Run when argument i arrives (staged), or only the last clause's body runs?

#### Round 4

- User: no statements, only an expression; agent should have reminded. O3 resolved by user; agent concedes it let O3 drop. Logged (22.11.26, 3).
- User: naive composition of multi-param lambdas with patterns explodes (3^N); how do pattern calculi and pattern-matching languages handle it? Baseline: listed cases only, error on others. Logged (22.11.26, 6).
- Agent survey: `patterns-survey.md`, logged (22.11.26, 6).
- Candidates (palette, no ranking):
  - C1 clause matrix in core: lambda = N params, ordered rows of N patterns plus one body each; first match wins; no match is an error. Tree keeps authored structure.
  - C2 one-level core: keep a unary three-arm shape split (today's triage, or a lambda clause); vf lowers multi-arg and nested patterns to nested splits (decision tree). Core stays small; structure is erased.
  - C3 failure as value: Jay-style `p -> s | r`; each case is a separate unary value; alternatives compose by extension; an unhandled failure at the top is an error.
- Crux K1: must compile-time code see the authored clause matrix in v0 (favors C1/C3), or is a vf-level record enough (favors C2)?
- Open: name vs fruit-literal pattern ambiguity: if a head `{x}` is a name, a fruit literal pattern needs a marker.
- User: too rigorous for this stage; write less. Agent: short replies, one point or question at a time.
- User picks C1, shape patterns only; M cases x N params; default whole-tree binding; discriminative patterns, any order; mixing shape and general in one position is an error; binders are identifiers. Logged (22.11.26, 3). K1 resolved toward C1. O6 resolved: binders carry identity.
- Note (user, ledger only): case-lambdas have many corner cases; rigorous specification pending.
- User sketch logged (22.11.26, 3): `[[{:lambda}], <ps>, ~[[~[{:all}, <id>], ~[{:fork}, <id1>, <id2>], ...], expr], ...]`.
- Agent: `~[{:fork}, <id1>, <id2>]` has three elements; a pair holds two. Needs `[{:fork}, id1, id2]` or `~[{:fork}, ~[id1, id2]]`.
- Agent: with order-free cases, after k < N arguments several cases can still match; private state needs the set of surviving cases, not one branch.
- User: fork pattern `~[{:fork}, ~[id1, id2]]`. Logged (22.11.26, 3).
- User (tentative): cases match in order as written; the old triage encoding materialized trees explicitly and avoided the surviving-set issue.
- Agent: order resolves overlap, not timing. Cases [fork, leaf] and [fork, fruit] both survive the first argument either way. Alternative: match only when all N arguments are bound; shape tests do not force, so deferring loses nothing, and private state is again just progress. Not logged yet; awaiting user.
- User: worried about devlog noise. Agent review: 14 entries today; (22.11.26, 2) and (22.11.26, 3) were low value; drafts (3), (4) could have stayed in the ledger. IDs cannot be removed. From now on: devlog gets decisions and surviving questions only; drafts and agent critique stay here.
- User agrees on matching at saturation; "lambda superposition, dead and alive until settled"; CLOSURE installed on the whole lambda. Logged (22.11.26, 4).
- Agent: no real superposition: arguments sit in a positional buffer in private state; nothing binds until the match picks a case.
- User: triage on partial lambdas breaks with per-case names (cannot tell which ids are bound). Moves to a general signature: `[[{:all}, id1], [{:triage}, id1, id2]]`; each body lists expected shapes for the triage params, e.g. `[{:fork}, {:leaf}, {:fruit}]`. Logged (22.11.26, 5), supersedes (12), obsoletes (13).
- Agent: fixed names per position restore per-argument env extension; the positional buffer from round (14) is no longer needed.
- Agent point: no wildcard shape in a case; a body indifferent to one triage position needs 3 duplicate cases. Allowing `{:all}` as a shape wildcard reintroduces overlap, hence case order.
- User: `{:all}` positions are the implicit wildcards; bodies list only triage shapes. (Agent meant a wildcard inside a triage position.)
- User design B: header is a row of bindings (one or two ids per position); each body carries a full row of expected shapes including an explicit wildcard; corner cases are errors.
- Agent: if overlap between cases is an error, B stays order-free and is strictly more expressive than A: [fork, *] and [leaf, fruit] do not overlap. Not logged; awaiting choice between A and B.
- User picks B. Logged (22.11.26, 3). User asks: what is the harm in allowing overlaps?
- User allows overlap. Logged (22.11.26, 4).
- User sketch: `[[{:lambda}, ...], <ps>, [<id1>, <id2>, <id3>, ...], ~[[{:all}, {:fruit}...], expr], ...]` with a flat identifier list.
- Agent: if a position's id count comes from each case's shape, the same id means different things per case, which reopens the partial-triage problem of (22.11.26, 5). If it comes from all cases together, adding a case can shift every later name. Suggest grouped header per position. Not logged.
- User accepts grouped header. Logged (22.11.26, 3). User asks whether discrimination is needed here.
- Agent: group length (one or two ids) discriminates; ids are already tagged trees. Rule: a fork shape on a one-name position is an error.
- User: length is enough. Folded into (22.11.26, 3).
- User: no triage form remains; private state = count. Logged (22.11.26, 3). Spec 02_v0.md still lists Triage and {:triage}; edit only on request.

### Topic 3: vf syntax for case-lambdas

Agent proposal (from snapshot grammar: `\[..]` closure_head, `?` triage, `~[a, b]` nyad):

    \[n, ~[l, r]]
        ?[_, fork] do f(l, r) end
        ?[_, leaf] do n end
        ?[_, _]    do fallback end

- header param: identifier (one name) or `~[l, r]` (two names); `?[...]` row of shapes `leaf`, `fork`, `fruit`, `_` + block.
- `\[x] do x end` unchanged: no rows = one all-wildcard case.
- `?` prefix needed: `[_, fork] do ... end` would parse as a list with a loose block postfix.
- User: `_` is an ordinary identifier; row heads hold symbols; no `do end`; `\[]` is a prefix form; after `?[]` expect one expression; no plain expression once a row is present. Logged (22.11.26, 7).
- Agent: (a) `:fork` already parses as a label-less labeled expression; symbol spelling needed (`{:fork}` works today). (b) ASI ends an expression at newline, so rows on later lines would end the lambda. (c) Nested case-lambdas: trailing rows attach to the innermost lambda (dangling else).
- User: `{:fork}` verbose but this is low-level; innermost attachment with parens; multiline lambda should parse whole. Logged (22.11.26, 7).
- Agent: ASI rule (snapshot l.226-237) ends a complete expression at newline; candidate amendment: a newline followed by `?[` continues the innermost case-lambda (one-token lookahead). Zero-change option: wrap in parens.

### Topic 4: forcing program in case-lambda syntax

- User: no ASI amendment; the devlog will be replayed when the syntax chapter is revised.
- Agent rewrote artifacts/20261120-172943/force.vf as `force.vf` here (not run). Triage applications become applied case-lambdas; tag-of destructures its parameter directly; forcing binders kept as in the original. Every multiline lambda sits inside brackets or parens, so ASI does not interfere.
- Agent: `force-end.vf` applies `end.` (20.11.26, 6) with the reading: close every open block up to the nearest enclosing delimiter, or all at top level. Not run.
- User: binder body is one expression, so `=[...] expr` without `do end`; sequential clauses can hold the work and the body is the result. Agent agrees; `force-flat.vf` (not run) assumes clauses bind in order (let*), as the snapshot example `=[x: .., y: x + ..]` implies, and that each clause forces its value. Not logged; awaiting confirmation.
- User confirms; `end.` is a proposal, not a feature. Logged (22.11.26, 8). Agent: with lambda and binder bodies as expressions, `end.` only applies to explicit `do` blocks.
- User: forcing by binders already logged; (22.11.26, 8) fixed to cite (23.10.26, 3). Keep `end.` in mind, never present as a feature.
- User: `\[e]` with a `{:fork}` row breaks (22.11.26, 3); agent had violated its own logged rule. Fixed in force.vf, force-end.vf, force-flat.vf by widening to `\[~[e, r]]`, keeping fork -> `~` as in the original.
- User: has read ZINC; update incoherent todos. ZINC-reading checked; (23.10.26, 5), (26.10.26, 3), (07.11.26, 3), (21.11.26, 2) superseded by (22.11.26, 9)-(22.11.26, 12).

### Topic 5: case-lambda construction experiment

- User: write fresh; unique stub OK; a vetochka program builds the trees. Done in case-lambdas/ (14 checks pass). Logged (22.11.26, 15); task (22.11.26, 12) checked. Open choices C1-C4 in case-lambdas/notes.md.
- User: wrap header in empty env (logged (22.11.26, 14)); identifiers are a rewriting optimization: holes vs already-rewritten trees, bound ids cannot be quoted (logged (22.11.26, 13)); inconsistency was in the throwaway case-lambda evaluator, not worth fixing.
- Experiment adjusted to (22.11.26, 13)-(22.11.26, 14); 15 checks; red-green on `observe` (old {fork}, new {fruit}). Logged (22.11.26, 15).
- User: C1 fine (logged (22.11.26, 3)); asked about C4; asked to try the reflective hole: done, logged (22.11.26, 15).
- User: apply rewriting to structure. Done; 18 checks; old evaluator fails `spines`. Logged (22.11.26, 13).
- User: the rule extends envs, not only replaces; do others do that? Agent (from recall, unverified): explicit substitution composition M[s][t] = M[s o t] (lambda-sigma, Abadi-Cardelli-Curien-Levy 1991); contextual modal type theory metavariables instantiated by substitutions (Nanevski-Pfenning-Pientka 2008), matching the holes lens; Kernel eval with constructed environments (Shutt); sets-of-scopes identifier binding (Flatt 2016). Open: precedence; spike lets parameters override the foreign env.
- User: precedence follows from unique ids; no separate rule.
- User: header env = defining env without own params, not empty. Applied; 18 checks. Logged (22.11.26, 14), supersedes (29).
- User: `unique` lets the language express cycles without tying the knot. Agent reading pending user confirmation (below).
- User: add devlog rule: before committing, condense the session entries in the commit. Added to devlog/rules.md (Write), scoped to uncommitted entries so committed IDs stay stable.
- User on cycles: just an intuition. Kept here, not logged.

## Devlog condensation

At the user's request the 22.11.26 section was condensed from 35 to 16 entries before the session ended; references in this record and the experiment files were remapped. Old to new: 0->0, 1->0, 2->1, 3->3, 4->3, 5->2, 6->2, 7->5, 8->3, 9->6, 10->6, 11->3, 12->3, 13->3, 14->4, 15->5, 16->3, 17->4, 18->3, 19->3, 20->7, 21->7, 22->8, 23->9, 24->10, 25->11, 26->12, 27->15, 28->13, 29->14, 30->15, 31->3, 32->15, 33->13, 34->14. The pre-condensation log is in git history (commit 8f1c60c).
- User: yes, align format.md with the condensing rule. Added the session exception to devlog/format.md.

### Topic 6: what is left before implementing v0; CESK abstract or specific

- Agent list and recommendation given in chat (abstract machine normative; cells, known fruits, ZINC batching as refinements checked by differential tests). Not logged; awaiting user.
- User: CESK implemented in code as in the spec, crosstest; logged (22.11.26, 16). fix experiment done, logged (22.11.26, 17). Store question answered in chat.
- User: factorial and Fibonacci. Done over unary numerals in `arith`; 27 checks. Folded into (22.11.26, 17).
- User: store keys are addresses `[[{:address}], <private-part>]`; agent misread earlier. Logged (22.11.26, 18).
- User on whole-value binding in a split: third name in a fork position vs rebuilding `~[h, t]`; leans to rebuilding. Agent analysis in chat; awaiting choice.
- User: log the fork direction (rebuild `~[h, t]`). Logged (22.11.26, 19).
