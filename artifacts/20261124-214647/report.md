# Symbols and colon blocks: corpus evidence

Status: **evidence for decisions already made**, 24 November 2026, (24.11.26, 6)–(24.11.26, 13). The user approved the change before this run and asked for the corpus evidence as well. The corpus, parser and measurements are the agent's. The Chinese text has not been reviewed by a native speaker.

## Files

| Path | Content |
| --- | --- |
| `vfs.py`, `test_vfs.py` | throwaway parser for the decided syntax, and its checks |
| `convert.py` | mechanical conversion of `artifacts/20261124-123200/corpus/` (labels with `\| ;;`) |
| `converted/` | converter output, unedited |
| `corpus/{en,ru,zh}/`, `probes/` | the converted corpus after hand edits |
| `experiment.py` | measurements; writes `results.md` and `results.json` |

Run `python3 -I test_vfs.py`, `python3 -I convert.py`, `python3 -I experiment.py`.

## Syntax measured

- `~name` is a symbol and lowers to the fruit `{:name}`. The tests check `~fork` against `{:fork}`.
- Blocks are `: ... ;;`. There are no labels.
- Loose application takes only a symbol (with its tight postfixes) or a block, and lowers like the parenthesized form: `f ~x` as `f(~x)`. Application is flat.
- Adjacency: a delimiter never touches its contents. `:` may touch a preceding symbol or closing bracket or brace. Otherwise, outside, a delimiter touches only the bracket enclosing its block, or a comma.
- `|` is an ordinary operator character. `#` is still used for line comments; its spelling is open.
- v0 forms parse as before, because vf includes v0.

## Conversion

`convert.py` rewrote each label using the earlier parser's payload extent:

| Old | New |
| --- | --- |
| `name: \|` | `~name:` |
| `name: [..]`, `name: (..)`, `name: {..}` | `~name[..]`, `~name(..)`, `~name{..}` |
| `name: payload` (anything else) | `~name(payload)` |
| any other `\|` | `:`, glued after `)`, `]`, `}` and after wrapped payloads |

Hand edits followed the user's clarification that v0 lambdas are not general callback syntax in vf (24.11.26, 13):

- **Symbol-named blocks.** Callbacks in 03, 04, 09, 12, 13, 14, 15 and 17 became symbol-named blocks, for example `for_each(xs) ~x: ... ;;` and `on ~unlock ~code: ... ;;`.
- **Symbols for names.** Event names, enum members, pass names and row heads became symbols (`~click`, `~debug`, `~collect_fn_names`, `?[~all, ~fork]`).
- **v0 code stays.** The v0 rows in 05 and 09 stay as v0 code with their own meaning.

## Results

Full tables are in `results.md`.

**1. Parsing.** All 51 programs and 3 probes parse. Each program has the same tree shape in English, Russian and Chinese once names, strings and numbers are erased.

**2. Size.** The new corpus is about 2% shorter than the labels-and-`|` version: 8027 → 7876 characters in English, 9369 → 9212 in Russian, 5879 → 5713 in Chinese. Tilde sigils and parentheses replace label colons and `|`.

**3. Where openers sit.** Of 120 openers per language:

| Position | Count |
| --- | --- |
| glued after a symbol (`~else:`) | 36 |
| glued after `)`, `]` or `}` | 67 |
| inside an enclosing bracket | 3 |
| spaced after a comma | 1 |
| **spaced after a plain identifier** | **13** |

The last row is the visible cost of the rule: `defer :`, `loop :`, `build :`, `row :`, `spawn :`, `limits :`.

**4. Mutations**, counted per language; all three languages give identical counts:

| Mutation | Error on that line | Silent |
| --- | --- | --- |
| glue a delimiter to a neighbour | 449 of 450 (1 tolerated: `;;,:`, allowed by the rule) | 0 |
| old label or YAML habit, `~ret(bool)` → `ret: bool` | 30 of 30 | 0 |
| `~else:` written as `else:` | 36 of 36 | 0 |
| `~` dropped from a loose argument (`case circle[r]`) | 66 of 66 | 0 |
| `~` dropped inside brackets or at statement start (`on(button, click)`, `?[all, ~fork]`, enum member `debug`) | 0 | 42 of 42 |
| `~else:` moved to the next line | 0 | 5 of 5 |

- **Delimiters and habits.** Every gluing slip and every old-habit slip gives an error on the line where it happens. The `|-1` case from the previous experiment is gone, because `|` no longer opens blocks.
- **Dropping `~`** is caught only where a bare word cannot stand. Inside brackets, or as a statement, a bare word is a valid identifier. Under (22.11.26, 13), an unbound identifier is a hole, so the mistake can surface only when an appliance or v0 meets that hole, not at parse time.
- **An alternative on its own line** parses as a separate statement. This is the ASI behaviour accepted in (24.11.26, 2).

## Reading notes

The agent's observations:

- **Symbol-named blocks read well for one parameter** (`walk(module) ~node:`) **and for several** (`fold(xs, 0) ~acc ~x:`).
- **A clause symbol next to a parameter symbol is positional.** In `connect(server) ~on_error ~err:`, nothing in the syntax says that `~on_error` names the clause and `~err` the parameter. The appliance's convention decides.
- **Named arguments are now symbol applications** (`~ret(bool)`, `~min(0)`), and data reads as records (`[~customer(o[.customer]), ~spent(...)]`).
- **Bare-word statements in blocks** are still valid identifiers, such as the struct fields `position :: vec3`. The convention that symbols name variants and members (`~debug`, `~circle[...]`) makes a missing `~` there silent; see mutation results.

## Open

1. **Missing-`~` slips are silent** in brackets and at statement start. Options: accept it as a semantic error, or give appliances a lint for unbound identifiers where symbols are expected.
2. **The line-comment spelling** is still `#` only by proposal.
3. **The spec text and the project parser** are pending, behind (22.11.26, 11).

## Limitations

- The corpus derives from the agent's earlier corpus by conversion and hand edits, not from real code.
- The parser is a throwaway.
- The mutations cover only the listed slips.
