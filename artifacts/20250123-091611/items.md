# Native and host work

- [ ] Implement designator recognition and the Continue/Dispatch/trap protocol, with malformed-form and saturation cases.
- [ ] Specify input evaluation for each initial opcode and its observable in-flight state.
- [ ] Specify and test i64 arithmetic, comparison, byte-string operations, and the agreed tag equality.
- [ ] Use the machine trampoline contract for calls into native C.
- [ ] Define the initial host services and recoverable-error data.
- [ ] Connect `parse_term` to the shared parser and encoder.
- [ ] Write a bootstrap v0 file that loads and runs a program.
- [ ] Reconcile the proposed entrypoint in TASK.md with opcode syntax and semantics, shared syntax in 20241220-144435, and function parameters in 20241225-140425.
- [ ] Run a hello-world file end to end, including observable output and failure handling.
