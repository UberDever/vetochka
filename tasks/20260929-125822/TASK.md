# Canonical form is not a fixed point

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- [fact] (agent, 2026-09-29) Formatting canonical text again changes it: `format("f(x,   y)")` is `"f(x, y)\n"`, and formatting that gives `"f(x, y);\n"`: the trailing newline comes back as `;`. Found while writing the first Lua test (`source_tree_format_canonical` through the Lua module's `format`).
- [fact] What is claimed today: the canonical form parses again (the former Zig test `source formatters smoke`, now in `tests/source_test.lua`). Nothing claims that formatting is idempotent.
- Open (user): should canonical form be a fixed point, `format(format(x)) == format(x)`? If yes: a test for it in `tests/source_test.lua`, and a fix in the formatter or the newline-based semicolon insertion.
