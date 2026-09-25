# Unicode identifiers

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, reducer
- EPIC: 20241220-144435

- Decision (user, 2026-09-24): ASCII-only identifiers are a drawback of the current implementation, not a restriction of the language.
- [fact] The spec's grammar still reads `identifier_start ::= [a-zA-Z_]`, and the lexer follows it: `Δ` is rejected.
- Open: which non-ASCII characters identifiers admit (the spec rule), then the lexer.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
