# Block delimiters across languages: corpus experiment

Status: **experiment and evidence, no decision.** 24 November 2026. This compares the `| ... ;;` candidate with per-language word pairs for the shared vf syntax of vmeta/vsystem, following (24.11.26, 0)–(24.11.26, 3). The user asked for a large corpus covering paradigms, use cases and forms in English, Russian and Chinese, and a comparison with the special word sets `do end`, `тут всё` and a Chinese pair. A word pair could be admitted instead of the symbol pair if its syntax proves too good to pass by.

The corpus, parser and measurements are the agent's. The Chinese text and the Chinese delimiter candidates have **not** been reviewed by a native speaker. The user does not know Chinese.

## Files

| Path | Content |
| --- | --- |
| `corpus/{en,ru,zh}/*.vf` | 17 programs per language, written in `\| ;;` with `#` line comments |
| `probes/{en,ru,zh}.vf` | natural identifiers that a word pair would reserve |
| `vfp.py` | throwaway parser for the replayed vf shape; delimiters are a parameter |
| `test_vfp.py` | parser sanity checks |
| `experiment.py` | renders, parses, measures; writes `results.md`, `results.json`, `renders/` |
| `distance.py` | how far from the mistake non-local gluing errors land |
| `renders/<lang>/<design>/` | native word renderings, plus `do end` in Russian and Chinese code |

Run `python3 -I test_vfp.py`, `python3 -I experiment.py`, `python3 -I distance.py`.

## Grammar used

The parser follows the devlog replay, not `artifacts/20261110-000001/new_spec/02_syntax.md` or the C parser, both of which are stale per the user:

- Lambdas `\[x] e` and binders `=[a = e, ...] body` take one expression (22.11.26, 8). Case-lambda rows `?[...] e` continue across newlines and attach to the innermost lambda (22.11.26, 7). A lambda needs a named parameter (19.11.26, 22).
- Blocks are first-class, loosely applied, and introduce no scope (30.08.26, 5), (13.11.26, 3).
- Field access uses selector paths, `v[.len]` (20.11.26, 1). A plain `v.len` is a glued-operator error.
- Infix chains are flat and need trivia on both sides. Prefix operators touch their operand (20.10.26, 1). There is no juxtaposition application.
- Strings are `{...}`. Typed literals use string application, `f64{3.14159}`. Symbols are `{:fork}`.
- ASI is completion-based. Newlines inside brackets are whitespace. `...` continues a line (27.10.26, 1).

Assumptions beyond the record:

1. **Unicode identifiers.** Letters of any script start an identifier, and letters and digits continue it. This is still open (07.11.26, 4).
2. **A lambda or binder may end an infix chain**, as in `f = \[x] x`. The retained grammar lacks this, but `artifacts/20261122-095953/force-flat.vf` relies on it inside binder clauses.
3. **A block may be a label payload**, as in `else: | ... ;;`. This is the proposal (24.11.26, 1).
4. **Adjacency rule, refined.** A delimiter never touches the block's contents. Outside, it may touch only **the bracket that encloses its block**, or a comma. Without "encloses", `f(x)|` would pass, because `)` is a bracket. The draft in (24.11.26, 3) said only "a bracket". Word designs get the same rule.
5. **Comments.** The punctuation design uses `#` line comments. Word designs keep `;;` comments, which is the status quo. Both keep `#| |#` and `#;`.
6. `@[...]` annotations are avoided, because (12.11.26, 5) leaves their status unclear.
7. **Localized types.** Per the user, the Russian and Chinese corpora localize types as well (`цел32`, `вещ64`, `указ(Т)`; `整32`, `浮64`, `指针(甲)`). Core symbols such as `{:fork}` stay ASCII, because they are registry names in v0, not types.

## Corpus

| # | Program | Paradigm or use | Forms exercised |
| --- | --- | --- | --- |
| 01 | guards | vsystem imperative | guards with early return; nested guard; closer on its own line |
| 02 | branching | control flow | `;; elif: (c) \|`, `;; else: \|`; while with continue; single-line if/else |
| 03 | functional | functional | lambdas, composition, binders; **no blocks** |
| 04 | callbacks | higher-order, events | blocks in calls ending `;;)`; callback inside a label payload |
| 05 | core-rows | v0 core | case-lambdas over shapes; **no blocks** |
| 06 | match | tagged unions | union body of labeled variants; match/case rows, multi-line and single-line |
| 07 | data | declarative | struct, enum, nested configuration |
| 08 | resources | error handling | `defer`, `errdefer`, `try`; one-line defer blocks |
| 09 | compiler-pass | vmeta metaprogramming | case-lambda rows, then blocks: `walk(m) on: \[n] \| ... ;;` |
| 10 | build | build DSL | nested declarative blocks with labels |
| 11 | tests | test DSL | `suite{...} \|`, a block as an argument, `expect_trap(\| ... ;;)` |
| 12 | state-machine | reactive | `on open: \| ... ;;` label blocks; if/else inside a handler |
| 13 | query | dataflow DSL | label chains joined with `...`; label payloads that are lambdas or blocks |
| 14 | actors | concurrency | `spawn \|`, loop, receive with case rows |
| 15 | ui | UI tree | deep nesting, inline `on_click: \| ... ;;`, explicit `;` inside a block |
| 16 | small-forms | edge forms | empty block, one-liners, blocks in lists, grouped block, comments next to delimiters, `#;`, `...` |
| 17 | deep-nesting | stress | six levels with an inner if/else |

Each program exists in English, Russian and Chinese with the same structure: 51 sources and 106 blocks per language. Every source is rendered into seven designs:

- `| ;;`
- `do end`
- `тут всё`
- `тут всё/все`: `все` also closes
- `开始 结束`
- `始 终`
- all pairs at once: every opener and closer word reserved together, each language writing its own pair

That gives 357 renderings.

### Samples

Russian, `тут всё`, from 12:

```vf
    состояние(заперта) тут
        при отпереть: \[код] тут
            если(код == секрет) тут
                перейти(закрыта)
            всё иначе: тут
                тревога()
            всё
        всё
    всё
всё
```

Chinese, `开始 结束` and `始 终`, from 04 and 02:

```vf
    逐个(列, \[元] 开始
        如果(元 > 10) 开始
            追加(结果集, 元)
        结束
    结束)
```

```vf
函数 分类: [数 :: 整32] 结果: 字串 始
    如果(数 < 0) 始
        {负数}
    终 否则如果: (数 == 0) 始
```

The same fragments in `| ;;` are in `corpus/`.

## Chinese candidates

Agent's proposals, to be checked by a native speaker:

- `开始 … 结束` ("begin … end"). Transparent, and familiar from textbook pseudocode. It takes the ordinary verbs "start" and "finish".
- `始 … 终` ("beginning … end"). Terse and literary, the pair from 始终 ("from start to end, always"). It is rarely a standalone identifier.

Not tried:

- `做 … 完` ("do … done"). Imperative, which the study avoided for blocks.
- Classical forms in the style of wenyan-lang, which (as remembered, not checked) closes blocks with 云云.

## Results

Full tables are in `results.md`.

**1. The delimiters leave no trace in the tree.** All 357 renderings parse and lower to the tree of their `| ;;` original. The core forms (03, 05) and the row-based half of 09 contain no delimiters at all: v0 code looks the same under every design.

**2. Gluing errors** (`results.md`, "Gluing mutations"; `distance.py`). Each delimiter was glued to its neighbour by deleting the whitespace between them:

| Design | Mutants | Error on the right line | Error elsewhere | Tolerated |
| --- | --- | --- | --- | --- |
| `\| ;;` | 396 | 394 (99.5%) | 1 | 1 |
| any word pair, any language | 397 | 253 (64%) | 143 (36%), median 6 lines away, max 24 | 1 |

- **Why words fail.** A glued word merges into the neighbouring identifier: `booldo`, `endnew_cap`, `trueend`, `真结束`, `结束如果`. The block then stays open, and the error surfaces where the parse finally fails. Gluing a closer to the following token is worst: 61 of 83 cases are distant.
- **No silent misparses** were seen in either design.
- **Chinese is the hard case for words.** Chinese text has no spaces between words, so writing `真结束` is the natural slip, not an unusual one. The one slip in authoring this corpus was a missing space before a Chinese label, `按钮({取消})点击时:`. With punctuation, the error appeared on that line.
- **The single distant `| ;;` case** is `else: |-1`. The opener merges with the prefix minus into the operator run `|-`. The lexer needs a rule here; see open question 2.
- **The tolerated case** is `;;,|`. The adjacency rule allows it.

**3. Collisions** (`probes/`). Code that uses natural names:

| Design | en probe | ru probe | zh probe |
| --- | --- | --- | --- |
| `\| ;;` | ok | ok | ok |
| `do end` | fails: field `end :: usize`, `s[.end]` | ok | ok |
| `тут всё` | ok | ok | ok |
| `тут всё/все` | ok | fails: `все(оценки, ...)` ("all") | ok |
| `开始 结束` | ok | ok | fails: fields `开始`/`结束`, call `开始(服务器)` |
| `始 终` | ok | ok | ok (`始终` stays one identifier) |
| all pairs at once | fails | fails | fails |

Reserving all pairs at once adds up every language's collisions in every file.

**4. Width.** Delimiters as a share of display columns, counting a Han character as two columns:

| Corpus | `\| ;;` | `do end` | native pair |
| --- | --- | --- | --- |
| en | 4.0% | 6.4% | (same as `do end`) |
| ru | 3.4% | 5.5% | `тут всё` 6.6% |
| zh | 4.3% | 6.9% | `开始 结束` 10.6%, `始 终` 5.6% |

**5. Typing.** Minimal input-mode switches over the code of each corpus, comments excluded. The layout tables were written from memory: Windows Russian ЙЦУКЕН, and a Chinese IME emitting half-width or full-width punctuation.

| Corpus | `\| ;;` | `do end` | native pair |
| --- | --- | --- | --- |
| ru | 637 | 733 | `тут всё` 525 |
| zh, half-width punctuation | 32 | 362 | `开始 结束` 32, `始 终` 32 |
| zh, full-width punctuation | 1297 | 1297 | `开始 结束` 1270 |

- **Russian.** `|` costs about 112 switches over 17 programs compared with `тут всё`, roughly 6.6 per program on a baseline of 525 caused by brackets, braces and `~`. `do end` in Russian code is worse than either.
- **Chinese.** With half-width punctuation, `| ;;` costs nothing extra. With full-width punctuation, the other punctuation dominates and the delimiter choice barely matters.
- The user has ruled that keyboard setup is a given when programming in Russian (24.11.26, 2). The numbers are recorded, not weighed.

**6. Stacking.** Every design has the same 72 closer-only lines and the same longest run of 6 closers, because rendering preserves layout. Words do not relieve stacking. The `end.` proposal (20.11.26, 6) would apply to either design.

## Reading the comparison

Agent's assessment, not a decision:

- **Punctuation is better or equal on every measured axis except Russian typing.** That axis has been ruled out as a cost.
- **What words offer is qualitative.** Russian code reads as prose: `если(...) тут ... всё иначе: тут ... всё`. That is the property the user's friends liked.
- **Words bring three costs:**
  - About a third of gluing slips report errors several lines away. This is likely worst in Chinese, where omitting spaces is the habit.
  - Some pairs collide with natural names.
  - Each admitted pair is a lexer and spec entry.
- **If a word pair is admitted, `тут всё` without the `все` alias is the only tested pair that passes the probes and is reviewed by its user.** `始 终` passes the probes but needs a native reviewer. `开始 结束` and `do end` collide.
- **How a file would choose its pair is open.**
  - Reserving all pairs everywhere fails every probe.
  - A per-file or per-version choice needs a declaration mechanism, which is new syntax or build metadata.
  - Admitting a pair next to `| ;;` gives two spellings of the most common construct.

## Open questions

1. **A Chinese pair, reviewed by a native speaker**, and whether the spaceless habit makes any Chinese word pair impractical.
2. **`|` merging into operator runs** (`|-1`, `|!x`). A block opener needs trivia after it, but the lexer reads `|-` first. The options keep the parser operator-policy-free:
   - end an operator run before a `|` that is followed by trivia;
   - forbid operator runs that start with `|` other than an explicit list;
   - leave it as a rare distant error.
3. **Full-width punctuation** (`：`, `；`, `（`). Should the lexer accept it as an alias, or reject it with a targeted message? This bears on Chinese whichever design wins.
4. **Script-homogeneous identifiers** would turn `x结束` into an error. They cannot help `真结束`, where both sides are Han.
5. **How to choose a pair**, if word pairs are admitted: per file, per language version, or never.

## Limitations

- The corpus was written by the agent in a single session. Its forms were chosen to cover constructs, not drawn from real code.
- The parser is a throwaway reconstruction with the assumptions above. Lowering is only compared across designs, not checked against a specified lowering.
- The mutations only delete whitespace. Missing or extra delimiters were not tested.
- The keyboard models were written from memory and were not checked on real layouts.

## Follow-up: rulings and Chinese speculation

Added after the results, the same day.

**User decision.** Use delimited `| ... ;;`, with no per-language word pairs. The user finds it decent overall, with strengths no other option has: code can look indentation-based ("tabulation larping"), and the syntax stays multilingual. See the devlog for the entry.

**`|` in operator runs.** The user considers `|-` lexing as an operator correct, for example the infix `a |- b` from logic. Open question 2 is closed: `else: |-1` is a user error that reports a non-local error, which the user accepts.

**Lambdas as infix operands.** This was assumption 2 above, and it is not a grammar restriction. In the retained snapshot, `tagged_expression ::= tag? (closure | infix<postfix_expression>)` makes a closure an alternative to a whole infix chain rather than an operand inside one, so `f = \[x] x` cannot be derived there, only `f = (\[x] x)`. Since (22.11.26, 8), lambdas and binders are prefix forms extending to the right, and `force-flat.vf` uses them as operands, so the snapshot lags the design. The user agrees that it is no restriction.

**Chinese word pairs: speculation only.** The user wants any Chinese pair chosen by a native speaker. These are the agent's non-native thoughts, recorded for that reader. Each pair was rendered and parsed, but none was measured.

| Pair | Source and reading | Risk |
| --- | --- | --- |
| `起 … 合` | 起承转合, the classical four-part composition of poems and essays: 起 opens, 合 concludes; 转 ("the turn") could be the localized `else:` label, so `合 转: 起` follows the composition's order | 合 alone can mean "combine" |
| `者 … 也` | the classical judgment frame "X者，Y也"; 若…者 is a classical conditional, so `如果(...) 者` reads naturally to a classicist | may read as wenyan cosplay to modern readers |
| `展 … 收` | unrolling and rolling up a scroll | 收 means "receive", a natural function name |
| `起 … 讫` | the compound 起讫 ("from start to finish"); 讫 is the character on the 收讫 ("paid in full") stamp | least poetic; 讫 is rare as an identifier |
| `【 … 】` | what a Simplified Chinese IME emits for `[` `]` with Chinese punctuation on; punctuation never merges into an identifier, so it avoids the gluing weakness of every word pair | a third, non-ASCII bracket pair; typing a list in Chinese mode would make a block |

Rendered with `起 … 合` and `转:`:

```vf
如果(密码 == 秘密) 起
    转到(关着)
合 转: 起
    报警()
合
```

Every word pair keeps the result in "Gluing errors" above. Chinese makes that slip more likely, because words are written without spaces.
