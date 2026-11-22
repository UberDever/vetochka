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
