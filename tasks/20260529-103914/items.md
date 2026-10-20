# Machine work and delivery order

The September 26 order remains current. Steps refer to the single owner of each subject.

0. [ ] Resolve value representation and the initial pith model in 20260926-091318 before machine C work.
1. [ ] Implement minimal CESK state, one-step `exec`, triage rules 0a..3c, identifier dereference, and hard traps here.
   Include Rule 3 on applications. Write the corresponding spec rules and run nyad-only and identifier cases to values.
2. [ ] Implement opcode dispatch in 20250123-091611 and bindings in 20241225-140425.
   Verify currying, saturation, shadowing, traps, sequencing, and constant-stack tail calls.
3. [ ] Implement native operations in 20250123-091611 with both literal forms from 20260926-091318.
4. [ ] Implement and test the initial pith inventory in 20260926-091318.
5. [ ] Complete host integration and the file-based hello-world case in 20250123-091611.

Additional machine decisions:

- [ ] State the departure from treecalcul.us: applications can reach Rule 3 and remain observable data.
- [ ] Specify who repeats `exec`, and how completion and traps reach its caller.
- [ ] Reconcile older evaluation-order drafts with the recorded branch-first direction.
- [ ] Settle metadata treatment without assuming observation versions are behavior-neutral.
- [ ] Check the old reducer defect against the current code before replacing its dispatch.

Recorded prerequisite (agent): the C-plus-Lua build migration completed on September 29. Its retained project constraints belong to 20260926-050130.
