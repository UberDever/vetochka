# vf block punctuation: November 2026 discussion

Status: design exploration, **not an accepted syntax change**. This note records the 23–24 November discussion of shared vf syntax for `vmeta` and `vsystem`, rather than v0 execution. Nothing below is parser-tested or normative.

Sources: [retained syntax](../20261110-000001/new_spec/02_syntax.md), [earlier vmeta/vsystem corpus](../20260903-120000/picture.md), [devlog](../../devlog/log.md), especially (30.08.26, Let) and the later syntax decisions. The current `do ... end` grammar remains the baseline.

## Objective and constraints

The original question was an ASCII block-opening delimiter of at most three characters, easy to type extremely often, that denotes a **block** rather than an evaluation step, arrow, or transition. The established `do ... end` are effectively English keywords. Removing them could make the shared syntax more natural for other human languages while retaining the same expression/lowering model.

The user values straightforward typing and readable code. Indentation would do most of the visual work; delimiters remain explicit and indentation was not proposed as syntactically significant. A short opener may be beneficial even if it is visually light. The user has **not** decided to replace `do ... end`; `| ... ;;` is a prominent candidate worth testing.

Candidate survey (evaluations from the discussion, not rulings):

| Opener | Finding |
| --- | --- |
| `|` | Current favorite: one character, looks like a boundary, not a direction; visually small, Shift-dependent on US layout |
| `||` | Stronger visual weight, but occupies familiar logical-OR spelling |
| `|;` | Echoes closing `;;`, but feels artificial and takes two distinct keys |
| `=>`, `->`, `|>` | Suggest consequences, flow, or pipelines rather than grouping/scope |
| `>>` | Prominent, relatively easy to type; suggests shift/redirection |
| `:`, `::` | Conflict with the existing label/type-expression surface |
| `,,` | Repeated easy key, but looks like a typo; rejected by the user |
| `..` | Repeated easy key, but strongly resembles range notation |
| `(: ... :)`, `[: ... :]` | Paired/structural, but more cumbersome to type |
| `{ ... }` | Familiar, but braces already encode literal UTF-8 bytes in vf |
| `▷`, `⊢` and other non-ASCII symbols | Initially considered as general beginning marks; unsuitable for a ubiquitous ASCII delimiter |

`;;` was attractive as a compact, easy-to-type closing delimiter; its conflict with comments is a separate lexer decision.

## The proposed shape

Assuming `| ... ;;` replaces only the **spelling** of a first-class vf block:

```vf
fn clamp: [x :: i32, lo :: i32, hi :: i32] ret: i32 |
    if(x < lo) |
        return(lo)
    ;;
    if(x > hi) |
        return(hi)
    ;;
    x
;;
```

Like `do ... end` today, a block should remain an independent expression and an ordinary loose-postfix application argument:

```vf
f(x) with: options |
    transform(x)
    emit(x)
;;
```

Here `fn`, `if`, `return`, and `with:` are interpreted by an appliance; vf does not gain those keywords. This is a test of shared syntax, not a v0 proposal.

## Standalone declaration groups and assignment

The user also brought back an earlier `let` design and proposed a bodyless, grouped declaration as a **statement inside an enclosing block**:

```vf
let[x = 10, y := 20] # a comment
```

Earlier: `let[clauses] do ... end` had a body and its own binding scope, with clause kinds written as infix operators; see the 30 August ruling and September experiment. The new example does **not** settle scoping or replace v0's later `=[...] expr` form. Questions include the destination scope of introduced names, precise meanings of `=` and `:=`, whether `var[...]` is a distinct head, and how declaration groups lower.

For vsystem-like examples, the session used `:=` for a mutable declaration, `=` for ordinary infix assignment or an immutable declaration *depending on context*, and `+=` for compound assignment. **No operator semantics or precedence was approved.** The intended frontend can compile infix expressions using its stable operator table. There is no need for a special `set` head.

```vf
fn binary_search: [xs :: list_t[i32], target :: i32] ret: i32 |
    let[lo := 0, hi := len(xs)]

    while(lo < hi) |
        let[mid = lo + (hi - lo) / 2]

        if(xs[mid] == target) |
            return(mid)
        ;;

        if(xs[mid] < target) |
            lo = mid + 1
        ;;
        if(xs[mid] > target) |
            hi = mid
        ;;
    ;;

    return(-1)
;;
```

## Stress-test corpus

Examples are **illustrative** of vf's existing application, labeling, grouping, annotations, and infix conventions with only the proposed block/comment spellings. Each appliance would assign its own semantics to the inert syntax.

### Functional and higher-order

```vf
fn transform: [xs :: list_t[i32]] ret: list_t[i32] |
    map(xs, \\[x] x * x + 1)
;;

fn collect: [xs :: list_t[i32]] ret: list_t[i32] |
    let[result := []]

    for_each(xs, \\[x] |
        if(x > 10) |
            append(result, x)
        ;;
    ;;)

    result
;;
```

A single-expression lambda is more economical than an inline block. `;;)` when a block appears in a call is valid only under the proposed grammar and is visually awkward.

### Declarative structures and matching

```vf
union Shape: [] |
    circle: [radius :: f64]
    rect: [width :: f64, height :: f64]
    empty: []
;;

fn area: [shape :: Shape] ret: f64 |
    match(shape) |
        case circle: [r] |
            f64{3.14159} * r * r
        ;;
        case rect: [w, h] |
            w * h
        ;;
        case empty: [] |
            f64{0}
        ;;
    ;;
;;
```

Rows remain ordinary head/label/block applications. The matcher and case-checking belong to the frontend. Several adjacent `;;` delimiters make deeply nested endings noisier.

### Resource handling

```vf
fn read_all: [path :: str] ret: !bytes |
    let[file := try(open(path))]

    defer |
        close(file)
    ;;

    let[size = try(file_size(file)), buffer := try(allocate(size))]

    errdefer |
        free(buffer)
    ;;

    try(read_exact(file, buffer))
    ok(buffer)
;;
```

`defer | ... ;;` reads cleanly as a loose block argument, without special syntax for `defer`.

### Structs, selectors, and a build DSL

```vf
@[layout: soa] struct Particles: [] |
    position :: vec3
    velocity :: vec3
    lifetime :: f32
;;

build |
    target name: {app} kind: {executable} |
        sources: [{src/main.vt}, {src/net.vt}]
        optimize: {release}
    ;;

    rule name: {no-raw-malloc} |
        forbid: [malloc, realloc, free]
        allow: [allocator]
    ;;
;;
```

The same surface serves imperative and declarative constructs; frontends can assign different meaning to the application trees.

### Conditional alternatives

Two competing *frontend conventions* were tried, with no new vf syntax selected:

```vf
if(ready) |
    send(packet)
;;
else |
    retry()
;;
```

The frontend would have to associate adjacent `if` and `else` expressions inside the block. Another variant embeds a labeled alternative through parentheses: `else: (| retry() ; ;)` is only schematic; the actual delimiter would need to be contiguous `;;`. This exposes how label payloads are tight, blocks are loose, and there is no built-in dangling-`else` rule.

### Deep nesting and small blocks

```vf
on_start | initialize() ;;
noop | ;;

transaction(db) |
    for_each(records) |
        if(valid(record)) |
            with_lock(mutex) |
                update(record)
            ;;
        ;;
    ;;
;;
```

This is the most conspicuous readability cost. Indentation gives the shape; repeated block terminators provide explicit parse boundaries but little visual information about which construct ends.

## Localization experiment

Replacing the only reserved English block words may allow the *same vf grammar* to be used with identifiers and label names in several languages. This is lexical/syntactic freedom, **not** a claim that natural-language grammar is magically portable.

English:

```vf
function name: factorial params: [n :: i32] returns: i32 |
    let[result := 1, i := 2]
    while(i <= n) |
        result *= i
        i += 1
    ;;
    result
;;
```

Russian:

```vf
функция имя: факториал параметры: [число :: i32] результат: i32 |
    пусть[итог := 1, множитель := 2]
    пока(множитель <= число) |
        итог *= множитель
        множитель += 1
    ;;
    итог
;;
```

Japanese, with a different order of labeled arguments:

```vf
関数 引数: [数 :: i32] 結果: i32 名前: 階乗 |
    定義[積 := 1, 因子 := 2]
    繰返(因子 <= 数) |
        積 *= 因子
        因子 += 1
    ;;
    積
;;
```

The Japanese example is syntactically illustrative; recognizing and normalizing the reordered labels would be an appliance responsibility. German/French/Arabic were also discussed. Arabic brings a separate right-to-left / ASCII bidirectional-rendering issue. The retained syntax chapter currently admits only ASCII identifiers: **Unicode identifiers are a prerequisite** for these examples and are not an implemented feature.

## Conflicts and unresolved questions

1. **Lexical conflicts:** `;;` currently begins a line comment and `|` participates in generic operator runs. Reserving a standalone `|` risks breaking `a | b`, cons/pattern notations like `[head | tail]`, and general operator extensibility. The vf lexer must decide block-vs-operator without consulting an appliance's operator table. Whitespace-sensitive disambiguation or another token spelling would each carry costs.
2. **Comment migration:** `#` as a line-comment introducer was proposed (`let[...] # comment`) if `;;` becomes the closer. Existing `#|` nested block comments and `#;` structured comments would need a non-ambiguous precedence rule; the proposal did not abolish them.
3. **ASI and multiline application:** The new delimiters must coexist with expression-completion newline insertion, `...` continuation, parentheses/brackets, and loose block application.
4. **Nested expression boundary:** `;;)`, runs of `;;`, inline callbacks, and two-arm conditionals warrant larger real-world corpus tests. `do ... end` has a readability advantage in some of these cases.
5. **`let[...]` scope:** A standalone declaration group is not simply the former `let[clauses] do ... end`. Determine binding visibility, shadowing, mutability operators, and annotation/statement conventions separately for vmeta/vsystem.
6. **Language neutrality:** Fewer reserved English words helps; Unicode identifier admission, editor support, bidi display, and translation of frontend-defined APIs are distinct issues.
7. **No decision to change grammar:** The user said the option looks reasonable and prominent, but explicitly remains unsure about replacing `do ... end`. This document is evidence for later evaluation only.

No parser, formatter, lexer, spec, or runtime changes are proposed in this PR.
