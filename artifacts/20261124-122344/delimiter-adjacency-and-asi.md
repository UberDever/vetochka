# Block delimiters: adjacency, layout, and ASI

Status: follow-up to [the delimiter study](../20261124-120700/vf-block-delimiter-study.md) and [the Russian follow-up](../20261124-120700/russian-block-words-followup.md), 24 November. Shared vf syntax for vmeta/vsystem. Not parser-tested. The user has said grammar changes are manageable and the current grammar can be altered on demand; it was not treated as a constraint here.

## Word pair versus punctuation: user's points

- Keyboard layout. The agent argued that `тут`/`всё` type without a layout switch while `|` (absent from the standard Windows Russian layout, as far as the agent recalled) does not. The user rejected this as a cost: programming in Russian requires keyboard setup either way, so `| ;;` is not much of a hurdle. The agent agreed: `[ ]`, `{ }`, `<`, `>`, `~` already require Latin input in Russian vf.
- The agent's arguments for punctuation, not contested: the Russian feel of the examples comes from identifiers and labels, not the delimiter pair; `все` would take a natural name for an "all" predicate; each language would extend the lexer and spec; punctuation frees `do` and `end` as ordinary identifiers (the user's example used `do({kaboom})`), so `label_word` reduces to `identifier`.
- The agent advised against admitting both `do ... end` and `| ... ;;`.

## Adjacency

User direction: forbid runs such as `x;;;;;` and generally gluing `;;`, while keeping `;;)` because it is natural to the user's taste. The user asked for a concise, understandable rule.

Agent-drafted rule (not yet confirmed by the user):

> A block delimiter never touches the block's contents. On the outside, it may touch only the bracket enclosing its block, or a comma.

The first draft said "only a bracket or a comma", which admits `f(x)|`; "enclosing" excludes it, matching the table.

| Text | Result |
| --- | --- |
| `x;;` | error: closer touches contents |
| `x ;;;;` | error: inner closer touches outer contents |
| `\| x ;;)` | allowed |
| `[\| a ;;, \| b ;;]` | allowed |
| `(\| x ;;)` | allowed: grouped standalone block |
| `\|x` | error |
| `f(x)\|` | error |

Trivia separates; block comments are trivia, as the user noted, so `x #|c|# ;;` is allowed, consistent with the infix-operator rule.

## ASI

The completeness-based ASI rule does not depend on delimiter spelling. The punctuation look invites two layouts that ASI ends early, each producing a valid program with a different meaning.

### Alternative on its own line

```vf
if(a > b) |
    a = b ;;
else: |
    c ;;
```

The newline after `;;` ends `if(...)`; `else: | c ;;` becomes a separate labeled block. Options considered:

- (a) Keep the closer and the continuing label on one line: `;; else: |`. No ASI change. **Chosen by the user.**
- (b) Existing `...` continuation after `;;`. Available, noisy.
- (c) A line starting with a label continues the previous expression. Rejected by the agent: labels are also block statements (union and build examples), so

  ```vf
  target name: {app} |
      sources: [...]
  ;;
  optimize: {release}
  ```

  would attach `optimize:` to `target` silently; restricting (c) to lines after `;;` still breaks this example.
- (d) Indentation decides. Makes indentation significant, which the study ruled out.

Resulting layout:

```vf
if(a > b) |
    a = b
;; else: |
    c
;;

if(a < b) |
    do({kaboom})
;;
return({stuff})
```

The user: "closer on its own line is good for non-else guard cases".

### Leading `|`

```vf
map(xs)
    | x ;;
```

ASI ends `map(xs)`, leaving a standalone block. The agent suggested a lint. The user ruled this a matter of familiarity, not a language problem, comparing it with readers who take `|` for bitwise or: "need to learn stuff". No language rule or lint was adopted.

## Open

1. Confirm or revise the adjacency rule.
2. Whether `| ... ;;` replaces `do ... end` is still not approved; the agent suggests applying the corpus-evidence condition of (30.08.26, 6) "Paren-statements" by analogy.
3. Lone `|` outside `operator_run`, `#` line comments next to `#|` and `#;`, and direct block payloads for labels remain as listed in the earlier artifacts.
