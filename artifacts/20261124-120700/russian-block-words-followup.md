# Follow-up: Russian block words versus symbolic delimiters

Status: **unresolved vf design alternatives**; no parser/spec/runtime change approved. Follow-up to [the delimiter study](vf-block-delimiter-study.md) on 24 November. This concerns the common vf syntax for **vmeta and vsystem**, not v0 semantics.

## The two directions

### A. A limited list of word delimiters

Keep `do ... end` and allow a **small, versioned list of synonymous opening and closing words**. The Russian pair preferred by the user is `тут ... всё`, with `все` (without `ё`) also a candidate closer.

| Role | Current | Russian candidate |
| --- | --- | --- |
| Open block | `do` | `тут` |
| Close block | `end` | `всё`, optionally `все` |

`тут` is locative (“here is the body”), not imperative (“execute”); `всё` closes the body without asserting evaluation. The words suit functions, conditions, declarations, cleanup and DSLs.

Benefits: existing ASCII English spellings can remain; symbols remain available for operators and comments; blocks stay first-class arguments with the same lowering. Costs: the common vf lexer becomes a multilingual reserved-word list, with admission/word-boundary/normalization choices, conflicts with ordinary identifiers and a maintenance problem as languages multiply. `все` is an especially common word and not generally interchangeable with `всё`. Whether opener/closer languages may be mixed, or must come in pairs, is undecided. Do **not** silently normalize `е` and `ё` everywhere.

### B. Punctuation, with no reserved block words

The leading candidate remains `| ... ;;`. Language neutrality comes from the syntax, and names/labels can be localized without teaching the parser words in each natural language.

Benefits: two short delimiter tokens, no expanding keyword inventory, visually light under indentation. Costs: `|` is already in vf's generic operator alphabet; `;;` already begins a line comment. A potential replacement `#` comment prefix would interact with existing `#|` block comments and `#;` structured comments. Deep nesting yields stacks of `;;`; callbacks may end with `;;)`. No lexical strategy was chosen.

Both directions still need Unicode identifier admission to write Russian ordinary identifiers. Neither makes the underlying DSL semantics linguistically independent.

## Russian word-delimiter corpus

Illustrative *proposals*, not programs accepted by today's parser. All heads and labels below are interpreted by vmeta/vsystem; the vf parser only sees applications, labels, infix expressions, and block expressions.

### Function, declarations and loop

```vf
функция имя: факториал параметры: [число :: i32] результат: i32 тут
    пусть[итог := 1, множитель := 2]

    пока(множитель <= число) тут
        итог *= множитель
        множитель += 1
    всё

    итог
всё
```

`пусть[...]` is a *new standalone declaration-group proposal* for the enclosing block, not an assertion that the earlier body-bearing `let[clauses] do ... end` is already changed. `:=`, `=`, `+=` remain ordinary infix data interpreted by a stable frontend operator table, with no special `set` construct.

### Conditional with a labeled alternative

```vf
если(соединение.активно) тут
    отправить(сообщение)
всё иначе: тут
    переподключиться()
всё
```

**Correction to the earlier example:** `иначе` is a **label**, not an independent command. The intended interpretation is application of the first block and then the labeled second block to `если(соединение.активно)`. Neither `если` nor `иначе` must be a reserved word.

**Important grammar distinction:** the retained `02_syntax.md` currently has `labeled_expression ::= (label_word NO_WS)? ":" argument_expression`, and the tight `argument_expression` does **not** include a bare block. Therefore the displayed `иначе: тут ... всё` is **not admitted by the retained grammar**, even after mechanically renaming `do/end`. The existing workaround requires grouping, `иначе: (do ... end)`. The intended revision would make a block a direct label payload, conceptually:

```ebnf
labeled_expression ::= (label_word NO_WS)? ":" (block | argument_expression)
```

This remains a **proposal**, not a decision or applied grammar change. It would also enable `else: do ... end` without parentheses under the English spellings. Preserve the existing distinction between tight label payloads and loose application/block postfixes elsewhere; test parsing and ASI rather than assuming this is ambiguity-free.

### Structures and variants

```vf
структура имя: Точка тут
    х :: f32
    у :: f32
всё

объединение имя: Результат тут
    успех: [значение :: i32]
    ошибка: [код :: i32, текст :: Строка]
все
```

The second ending intentionally illustrates the proposed `все` alias. It is not established as valid.

### Pattern matching

```vf
функция имя: описание параметры: [результат :: Результат] тут
    разобрать(результат) тут
        случай успех: [значение] тут
            печать(значение)
        всё
        случай ошибка: [код, текст] тут
            печать(текст)
        всё
    всё
всё
```

Rows are frontend conventions composed from labels and loose blocks, not special vf syntax.

### Resource cleanup and callbacks

```vf
функция имя: прочитать параметры: [путь :: Строка] тут
    пусть[файл := открыть(путь)]

    отложить тут
        закрыть(файл)
    всё

    прочитать_всё(файл)
всё
```

```vf
для_каждого(числа, \[число] тут
    если(число > 0) тут
        печать(число)
    всё
всё)
```

Nested closing words and `всё)` should be compared against symbol-only `;;` and `;;)` on representative corpus. `прочитать_всё` remains one identifier; a delimiter must be recognized as a whole token, not a substring.

### Declarative build DSL

```vf
сборка тут
    проект имя: {приложение} тут
        исходники: [{main.vf}, {net.vf}]
        оптимизация: {release}
    всё

    правило имя: {безопасность} тут
        запретить: [malloc, free]
        разрешить: [allocator]
    всё
всё
```

A natural fit for non-imperative blocks: `тут` merely introduces data in the block.

## Decision boundary

The meaningful fork is **small fixed multilingual word pairs** versus **language-neutral punctuation**. A configurable list of arbitrary words would be a third, more complex design and was not requested.

Open questions:
1. Exact reserved vocabulary and whether `все` aliases `всё`; allow mixed `do ... всё` or require language-matched pairs?
2. Unicode identifier categories, exact token boundaries, parser/highlighter behavior, and formatting policy for aliases.
3. Direct block payloads for labels (`иначе: тут ... всё`) without parentheses, including ASI and association with preceding applications.
4. Symbolic `|` versus generic operators; `;;` versus line comments; fate of `#|` and `#;`.
5. Comparison on real nested and inline code rather than isolated attractive examples.

**No option was selected and no implementation change is authorized by this note.**
