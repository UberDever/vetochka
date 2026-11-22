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
