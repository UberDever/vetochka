# First-class heads for special kinds

- STATUS: OPEN
- PRIORITY: 100
- TAGS: optimization
- KIND: TASK
- PARENT: 20260925-134730

- (user, 2026-09-24) Which kinds get their own head, like `{:closure}`, and which go under a category with the kind in `meta`. Like core opcode shorthand versus `$ns: {namespace} op: {opcode}`.
- (user+agent, 2026-09-24) Rule of thumb: a kind is special enough when the machine or Rule 3 dispatch needs it on the first node.
- (user, 2026-09-24) Related: storage may compress trees, for example not storing string tags in list entries. That may only affect what Rule 3 sees, not meaning.
- (user, 2026-09-26) Designators are short special strings at the head of a list; which of them the evaluator recognizes fast is this optimization question.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
