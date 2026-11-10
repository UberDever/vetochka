# Development log format

This format forks muh-todo's Markdown format:
https://github.com/UberDever/muh-todo/blob/main/docs/format.md

This document defines our format independently.

The development log is the project's single source of truth for development history.
Treat it as a write-ahead log (WAL): replaying entries in order must recover the
development state and how it changed. `devlog/` retains the full history, including
the two years of existing entries and all future rotations.

## Storage

UTF-8 Markdown. Preserve unrelated text, an existing BOM, and line endings.
The current file is `devlog/log.md`; rotations are `log.md.1`, `log.md.2`, etc.
Older rotations may be compressed. Retain all history. Rotate only between date sections.
`devlog/preferences.json` holds rotation parameters and tool defaults.
Artifacts live under `artifacts/<HUID>/`, where HUID is `YYYYMMDD-HHMMSS`.

## Dates and identity

A section starts with an exact, unindented `### DD.MM.YY` line without trailing whitespace.
Use ASCII digits and valid, zero-padded dates; `YY` means `2000 + YY`.
Dates strictly increase across the logical log. Same-day additions extend the current section.

An entry's ID is `(date, index)`: its section date and zero-based entry index.
Entries may change in place. Preserve existing IDs: no deletion, reordering,
or insertion before existing entries. Append new sections and entries.

## Entries

Recognize entries only within date sections, at column zero:

- `- `: note.
- `- [ ] `: incomplete task.
- `- [x] `: completed task.

Other bracketed checkbox forms are invalid. Markdown links remain valid note text.
Parsing is line-oriented, including inside code fences. Indented or quoted entries do not match.

Payload order:

```text
[tags] [Title. ] [relation statement] body
```

Brackets indicate optional parts. Use ASCII separator spaces. The body must be
nonblank and occupy one physical line; continuation lines are unrelated Markdown.

Tags use `(#tag #tag)`, with exactly one space between tags and none inside the parentheses' edges.
Identifiers contain Unicode letters, decimal digits, or `_/.!?+*<>=:-`.
Preserve order, duplicates, case, and spelling. Tags are explicit; omission means none.
The first all-decimal tag sets priority, clamped to 0–99. Default zero means no priority.
Malformed leading `(#` metadata makes the line invalid.

A title is one nonempty word without whitespace, control characters, or periods,
followed by `. ` and nonempty text. Otherwise, the text has no title.
Remaining text is literal body text, except for an optional leading relation.

## Relations

After the optional entry title, before the body:

```text
Supersession for (date, index) "target title".
Link to (date, index) "target title".
```

The quoted target title is optional; omit its quotes too when absent.
The ID identifies the target. The title is descriptive.
`Supersession for` replaces the target; mark that target `#superseded`.
There is no `#supersession` tag. `Link to` is a generic reference with no further prescribed meaning.

```markdown
### 10.11.26
- (#syntax #superseded) Draft. Original spelling.
- (#syntax) Revision. Supersession for (10.11.26, 0) "Draft". Revised spelling.
- Review. Link to (10.11.26, 1) "Revision". Check examples.
```
