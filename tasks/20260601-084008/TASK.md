# do-blocks as standalone expressions

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, rejected
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-05-31) Proposed `do: block_list? end` and `:do expression?` as standalone greedy expressions.
- Rejected (user): keep them as suffixes, for a clear primary and postfix split.
- Later (user, 2026-08-30): bare `do ... end` became a standalone entry lowering to `[{:block}, ...]`.
