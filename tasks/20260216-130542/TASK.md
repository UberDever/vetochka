# Application by cell adjacency

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, rejected
- KIND: TASK
- PARENT: 20241225-125809

- (user, 2026-02-16) Two adjacent nodes in cells apply to each other, so the reduce stack needs no apply token.
- Rejected (user): results of rules 2 and 3c can be scattered, so adjacency can't encode them.
- Later (user, 2026-05-31): applications are stored as explicit nodes.
