# Bytecode VM with DP register and ENV stack

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, rejected
- EPIC: 20241225-125809

- (user, 2026-01-31) A bytecode over cells: `PUT`, `APPLY`, `DROP`, `FORCE`, `BIND`, `LOAD`, `SCOPE_ENTER/EXIT`, with a `DP` register and `ENV` stack.
- Rejected (user, 2026-02-01): effects outside the reducer are invisible to the calculus. Features moved into opcodes the reducer recognizes.
