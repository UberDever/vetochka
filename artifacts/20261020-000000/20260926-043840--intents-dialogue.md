# Intents dialogue

Lightweight: (listen + question), (generate + attack), (converge + record).

## Framing
- Question (agent, confirmed by user 2026-09-26): which desired conditions, each with its why, root the task tree so every record gets one justified parent.
- Reversible (text in git; checkpoint `2259655`), but foundational.
- Constraints [fact] (tatr skill): intents are solution-independent; categories are tags; the agent does not invent intents; unplaceable records stay provisional at the root.
- Source (user, 2026-09-26): intents come primarily from `docs/new_spec/01_intro.md`, which the user fully agrees with.

## Model (confirmed 2026-09-26)
- (user) Intents come from the intro's characterising statements S1–S6; the motives M1–M4 are the why behind them, high-level enough to shape direction, not individual work.
- (user) Motives and intents sometimes blend. (agent) Test used here: a motive phrased as a condition you could check on the project (e.g. M3, a new C frontend with meta capabilities) is intent material; one about the author (M1, exercising skills) stays a motive.
- (agent) Medium-confidence item carried into pass 2: S4–S6 name the means (v0, vmeta, vsystem), so they may be initiatives under an intent rather than intents.

## Candidates (agent, pass 2, 2026-09-26)
- I1 (S1; why M3, M4): one syntax, expressive for people and trivial for programs to analyze and construct.
- I2 (S2, S3; why M4): any program can observe and build any program as ordinary data, without quotation.
- I3 (S4, S5; why M3, M4): new languages are defined by programs over a shared executable core, not by changing the core.
- I4 (S6, porosity; why M2, M3): C gains a modern frontend with compile-time meta capabilities; output stays ordinary C for the existing toolchain.
- Means, not intents: v0 → initiative under I3; vmeta, vsystem → initiatives under I4. Porosity and "as above, so below" are principles, not records.
- Palettes: fine (6+ intents, one per statement), medium (I1–I4), coarse (I1–I3 merged, plus I4).

## Objections
- Merge I1 into I2 (coarse): standing — grammar work (ASI, labels, gluing) is justified by human ergonomics, not reflection.
- I3 "any compiler to any language" not checkable: mitigated — aspiration in I3; v0's initiative carries a concrete done-when.
- Porosity also serves I2 (C enters as data, 2026-08-30): mitigated — secondary benefit in the body, one parent.
- Antithesis, implementation intents ("fully specified", "implementation matches spec"): refuted as intents — they are about means; those records go under initiatives.

## Cruxes
- Open: the hand-written spec as a whole has no intent in the intro. Split its work by what each part defines, or add a spec intent?
- ~~Open: the hand-written spec as a whole has no intent in the intro.~~ resolved (user, 2026-09-26): place spec work by the concept it defines (AGENTS rule); no spec intent.

## Decision (user+agent, 2026-09-26)
- Intents I1–I4 as drafted in Candidates ("these are fine", user).
- Initiatives: v0 under I3; vmeta and vsystem under I4.
- Spec work goes under the intent or initiative of the concept it defines. The hand-written-spec epic dissolves into the `spec` tag; its process rulings move to AGENTS.md. "Explain each inspiration source in the intro" stays a provisional root task.
- Given up: no single record overviews spec work (use `tatr ls :spec`); vmeta has no intent of its own, only as the means to vsystem.
- Next: a record-by-record mapping (KIND, PARENT, where each duplicated claim lives), shown to the user before any move.
