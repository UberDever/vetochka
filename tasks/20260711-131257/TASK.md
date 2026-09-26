# Write the v0 CESK rules

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, spec
- EPIC: 20260529-103914

- The spec's v0 CESK section has notation only.
- Inputs: the machine epic's rulings, binders (task 20260830-165615), the July catalogue in `docs/spec/03_v0.md`, and archive/20260613-120000/minimal-machine.md.
- Obligations: proper tail calls, a frame invariant, and native trampolines with no machine re-entry from C.
- ~~Open (agent, 2026-09-25): evaluation order decides whether the Rule 3 exception for pending applications (2026-08-30) can ever trigger: branch-first evaluates the operand before Rule 3; callee-first (July `RUN-APPLY`) retains it.~~ dissolved by the 2026-09-25 ruling below.
- Decision (user, 2026-09-25): Rule 3 never evaluates. Applications persist and Rule 3 observes them like any node (arity 2); identifier dereference is its only indirection, which is how programs use binders' non-locality with Rule 3. To inspect a result rather than the computation, bind it with `$fn` and inspect the identifier. This departs from treecalcul.us, where an application cannot reach a Rule 3 branch, and the spec should say so.
- Reminder for the user (2026-09-25): state in the spec (triage calculus section of `03_v0.md`) that Vetochka departs from treecalcul.us: applications persist and Rule 3 observes them instead of evaluating them first.
- (user, 2026-09-25) Consequence of the Rule 3 ruling: inspecting the result of a triage inside another triage takes explicit binding, since Rule 3 sees the inner application, not its result.
- ~~(agent, 2026-09-25) Scope of that cost: `$fn` functions evaluate their arguments (spec, `to:` step; plain application eager, 2026-08-30), and execution continues until a value, so one `let[r = t1(v)] do t2(r) end` suffices per inspected result. Nyads take raw operands (rules 0a, 0b, 2), so values built by applying nyads can hold unevaluated applications in their branches; fully computing such a tree needs a deep force: a Rule 3 walk that binds each branch, likely a library routine. A language-level `eval` stays excluded (2026-07-11; `exec` is host-only).~~ corrected (user, 2026-09-25): nothing requires an application-free tree. A value whose branches hold applications is finished; those applications are data. A consumer binds a branch only when it wants that result. No recursive evaluation follows from the semantics; the "deep force" assumed Jay's normal forms.
- Decision (user, 2026-09-26): `exec` drives the computation one step at a time. Given the evaluation rules, one applicable rule is chosen and executed. No deep evaluation: the CESK rules define evaluation completely, with corner-case prose where needed, as with the grammar.
