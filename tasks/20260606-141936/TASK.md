# Newline-based semicolon insertion

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax
- EPIC: 20241220-144435

- Done (user, 2026-06-05): newlines replace most `;`, and `...` continues a line.
- Current rule (spec): a newline becomes `;` when the context is layout-active, `...` doesn't suppress it, and the previous token ends an expression.
