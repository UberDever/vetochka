This would be a project development log to concisely write the current state of affairs alongside experiments (and their results).

⬜ ✅ ❌

### 25.12.24
- On fast (relatively) and compact (relatively) representation:
- Firstly, need to somewhat use cache locality => store tree/commands in the contiguous memory
- Idk how to represent `tree-calculus` trees as commands or something, so stick to the tree
- Node of the tree should be as small as possible => stick to a number (64-bit).
    Side note: actually, since we are storing *and evaluating* (adding nodes) of the tree, 
    nodes can't be implicitly linked by their position in the memory, because
    this would require copying them around. This means that we should store link information 
    explicitly, in the nodes themselves => this strips the restriction of uniform node size.
    Which means that node can be of any size and can be actually VLE encoded.
    But 64 bit nodes and single array is pretty simple implementation, so I stick to it.
- Node can be encoded as follows:
    | Node type           | 0..1 | 2..32 | 33..63 |
    |---------------------|------|-------|--------|
    | Tree node           | 00   | lhs   | rhs    |
    | Application node    | 01   | lhs   | rhs    |
    | Intrinsic node      | 1\*  | \*    | \*     |

    For `tree node`: lhs and rhs should be relative (in memory)
    offsets with reserved value `2**31 - 1` which indicates absence of the node.
    For `intrinsic node`: This node can store any data that is intrinsic for a
    interpreter i.e. built-in functions and variables.
---
- About scopes:
- Declared as `scope [name: string | none] do ... end`
- Named scopes are `modules` and unnamed ones are alternative form of `let-bindings`
- Inside: series of sugared bindings of the form
    See [`project/scope_syntax_example.md`](project/scope_syntax_example.md)
- Ending expression of the scope is final value of the expression. If no
    expression is provided, false/nullish idk value is returned (maybe 
    do an analysis before? not for interpreter...)
- No recursive and mutually-recursive bindings are currently considered
    - They are achieved via combinators and stuff
- Bindings are **not** calculated as statements, they are calculated on demand/use
- This notion of scope is not very mathematical, rather functional-flawored.
    This is because scopes are not first-class and are not part of the language
    internal representation (i.e. `tree-calculus` trees).
    But this notion seems functional enough, also easy to implement.
    The more rigorous implementation would consider scopes as a first-class entities
    with ability to combine and unpack them in several ways. And I'm afraid there
    would still be a problem of integration with filesystem
---
- About modules
- Create a `root.tree` in the project
- ~~In case this root is empty -- all paths will be relative to this root~~ The set of modules
    `M` is composed of every module found in every file that can be found recursively from the `root.tree` directory
- Bad decision: ~~Otherwise -- this root contains code that can be executed on setup (this is just
    a room for extension, i don't think this will be actually used)~~.
    Better to use `root.tree` as anchor for module location and nothing more.
    Otherwise, we blend together solutions (build time code execution and module location)
    which is not orthogonal.
- Interpreter accepts:
    - Single source file only and evaluates it top-down
    - (1) Set of modules to `import` and (2) `entry point` source file. First is defined by the pair of
        `(prefix, root.tree)` where `prefix` is the string that will be prepended to every module name on import and
        `root.tree` is an anchor to find all the project modules.
    - This way interpreter can be fed with any project with any naming as long as this project has **locally** unique
        naming for modules.
    - Example: `tree --prefix std/ --root-tree ~/dev/tree/std/root.tree my-script.tree`
        ```elixir
            use {std/bool} do
                true? true # Outputs true
            end
        ```
- Side note: Interpreter accepts either (1) source file to interpret or (2) path to `root.tree` and
    `entry point` source file. The former doesn't allow for `use` constructs because
    there is no explicit module locator prefix and I don't want to provide implicit heuristics.
    `root.tree` stands out as a way for linking the interpreter and project
    file system. This way is done through a file because it is "conventional?".
    Other way is to use shell variable like `ROOT_TREE` which can be used in the interpreter
    in similar fashion. I prefer the file tho.
- Module name is a string. Since all the modules that are accessible from the `root.tree` are assumed to
    be in programmer's control, it is their responsibility to come up with unique name for each module.
- Module is just a named scope. Reference to named scope can be done in every other
    scope, even in separate file. Project is just a certain collection of files,
    each containing scopes and their usages (inside the scope). Project is defined by `root.tree` in the project directory
- All modules created as follows:
    ```elixir
    scope {something} do
        ...
    end

    ```
- ~~Nesting of modules can be added as a syntax sugar, but I don't see if it's necessary. Right now modules
    considered to be a linear sequence of declarations in the file.~~
    Nesting of modules does nothing to their names, instead, it just brings outer definitions to the scope of
    the inner module
    See [`project/module_nesting_example.md`](project/module_nesting_example.md)
- All modules can be used as follows:
    See [`project/module_use_example.md`](project/module_use_example.md)
- `use` construct can enclose any expression, and since `scope` is an expression on itself, `use`
    can appear on top level with unrestricted embedding depth.
- `use` brings in top-level declarations (i.e. let bindings) into the scope of the expression
    following `in`.
    This can be a little clunky because it semantically can be considered
    as qualified prefix and every use of such prefix is then
    `use {some/path.tree} in func foo bar` rather than `path.func foo bar`.
    But this really simplifies module system.
    If you have a name clash you can alias the clashing things yourself.
    See [`project/module_name_clash_example.md`](project/module_name_clash_example.md)
- The `module` can be described as a syntax sugar. It acts as a "statement" that doesn't return
    anything. This makes language less consistent, but much pleasant to work with.
    See [`project/module_syntax_sugar_example.md`](project/module_syntax_sugar_example.md)
- If module is defined at top-level, implicit clause is created
---
- Let-bindings
- Since we have scopes, I decided to reuse this construct for let bindings also.
    Honestly, this feels natural from a semantical point of view, need to see it in practice
- Therefore:
    `let a = 10; b = 15; c = 20 in a + b + c; end`
    =>
    `scope do a = 10; b = 15; c = 20; a + b + c; end`

### 23.01.25
- on tagging
---
When I tried to imagine the way for language to communicate with the interpreter, I've stumbled upon
the fundamental decision -- how pure calculus can communicate with the underlying hardware?
It splits into two subdecisions -- whether calculus has interactive communication with hardware or not.

If it hasn't, then the only result that is observable is the state of interpreter when the calculation
is finished. This implies that we always can enforce the format of this result on the programmer.
My initial idea was to enforce the list of bytes (string) as the only format interpreter could decode from the computation.
This is limited, but in this case the only channel of communication between calculus and hardware is some static state.

If calculus has interactive communication it means that we must introduce some sort of interpreter intrinsic to it.
Then, using such intrinsic (i.e. `evalcall`) we can effectively notify an interpreter and tell it to do something to
the current calculation state. It's like a syscall in the OS kernel. Then, the things are a bit trickier
because for this to work we **want** not just printing, but other "actions" like changing the interpreter state.

This is when I started to ponder about value tagging. Functions are taggable (see tree-book) and
they preserve the functional behavior (they can be called normally despite the tagging).
But the value can't be "transparently" tagged. Value meaning is it's structure and nothing more.
Therefore, to tag a value is to dramatically change it -- on every usage of the value we must
ensure that it is boxed/unboxed consistently.

And there is two ways to do it: internally in the calculus or natively in the interpreter.
The former is way less efficient of course, the latter binds some parts of native implementation and
the whole language -- dramatic change for the language runtime as a whole.
Let's consider both approaches.

`Internal` approach allows to encode tagging in the calculus and code related operations in the calculus only.
The "example" of such approach in the code:
See [`project/internal_tagging_approach.md`](project/internal_tagging_approach.md)

Note that this approach implies that numbers are defined in the source code, when in reality the interpreter
encoder can just construct numbers before interpretation. In either case the information about the structure
of data is preserved in the calculus. Interpreter in this case "obeys" the calculus since it must
encode the value `3` as calculus expected.

`Native` approach is the other way around -- calculus "obeys" the interpreter and asks it in some
cases. The "example" is the code:
See [`project/native_tagging_approach.md`](project/native_tagging_approach.md)

It is clear that `native` approach is the way to go, since it is faster and more convenient.
The only downside is that we "extend" the channel of communication between calculus and hardware, effectively
increasing coupling to the implementation, reducing extendibility and portability.

AFAIK the must-have predefined stuff is the natural numbers -- keep the semantics of numbers
as close to pure calculus as possible while also doing everything in the interpreter. If we have
intrinsic numbers and their operations, then (hopefully?) we don't need anything else to make native.
Of course there are also floats, pointers/references, objects and closures, but for the current
project the numbers will suffice for now.

That said, we can tag the values using intrinsic numbers as tags, like so:
```
    a = 5 # single node: |[tag_int][5][tag_data]|
    s = {abc} # actually a list of numbers [97, 98, 99] or ^ 97 (^ 98 (^ 99 ^))
```
It greatly improves performance and space-efficiency.

---

### 31.01.26
- (#lang 0)
- lang 0 (aka Vetochka 0) is the minimal VM that (1) adheres to triage-calculus (the variant of tree-calculus); (2) has sufficient bytecode instructions to implement a high-level functional language with reasonable performance
- `cells` is a byte array; It serves as a VM memory; `cells` are indexed by `index`
- `cells` store program state as a collection of binary trees with optional payload bound to nodes
- ❌ A binary tree consists of several `node`s:
    * Reference `ref`: 00[6bit] | 01[62bit]; It is an `index` (short or long) into `cells` array, relative to the ref position; Has no children. Notation: `#**`
    * Tree0 `leaf`: 11000000; It represents a single Δ from triage-calculus; Has no children. Notation: `^**`
    * Tree1 `stem`: 11100000; It represents a delta node, followed by the `ref` node and then empty `*` node; Has one left `ref` leaf. Notation: `^^*`
    * Tree2 `fork`: 11010000; It represents a delta node, followed by the `ref` node and then another arbitrary node; Has two children, left one being a `ref` node. Notation: `^^^`
    * Native0 fixed `n0f`: 10000[59bit]; It represents an arbitrary number; Its signess depends on the interpretation; Has no children; Notation: `#**`
    * Native1 fixed `n1f`: 10010[59bit]; It represents an arbitrary number with a left child; Has a left child that is always `ref`; Notation: `##*`
    * Native2 fixed `n2f`: 10100[59bit]; It represents an arbitrary number with two children; Has a left child that is always `ref` and another arbitrary node. Notation: `###`
    * Native0 var `n0v`: 10001000[ULEB128][payload]; Represents an arbitrarily lengthed (ULEB128 length) blob of bytes; Has no children; Notation: `$**`
    * Native1 var `n1v`: 10011000[ULEB128][payload]; Represents an arbitrarily lengthed (ULEB128 length) blob of bytes with a single child; Has a single child that is always `ref`; Notation: `$$*`
    * Native2 var `n2v`: 10101000[ULEB128][payload]; Represents an arbitrarily lengthed (ULEB128 length) blob of bytes with two children; Has a left child that is always a `ref` node and second arbitrary child; Notation: `$$$`
- VM has a `reducer`. It is a term rewriter (or rather just writer, since it doesn't change terms inplace); It works with `cells` directly and has an `reduce` stack and `result` `index`
- `reduce stack` is an array of `index`ies that is used to schedule application order of the trees. It also can contain the `REDUCE` marker. It is used to delimit the applications
- `result index` is the index of the node that is always a `value` (aka `program`)
- Implementation detail: `stash` is an array of `index`ies that is used to store the intermediate application results during steps of reduction to a value (repeatedly `step` on the `term` at the `index`)
- `reducer` is basically an implementation of triage-calculus already, since it computes arbitrary tree expression
- ❌ (and below) To be able to represent more than expressions (and be somewhat performant), VM introduces its own bytecode, stack and registers
- Currently, Bytecode operates on `DP` register and `K` stack
- `DP` register is the `index` of currently reduced/reducing term; It is used as current `data pointer`
- `ENV` stack is TODO
- Bytecode instructions:
    * `PUT <node>`: put `node` at current `DP` in `cells`; put `DP` in `reduce stack`; shift `DP` to next vacant cell; a `node` is any of the nodes described above
    * `APPLY`: put `REDUCE` marker in `reduce stack`
    * `DROP`: reset `DP` to point at the next vacant cell that will be used to calculate next term
    * `FORCE`: run `reducer` at current `DP` until the result of reduction is not `value`; bind `DP` to point at this `value`
    * `BIND`: push current `DP` to `ENV` stack; `DP` remains intact
    * `LOAD <i>`: put `i`th `index` from `ENV` to `cells` at current `DP`; put `DP` in `reduce stack`; shift `DP` to next vacant cell;
    * `SCOPE_ENTER`: put `MARK` marker at the `ENV` stack
    * `SCOPE_EXIT`: pop every `index` from `ENV` stack until `MARK` marker (including this marker); it is an error if marker is not encountered and `ENV` is empty

### 01.02.26
- (#lang 0)
- I've encountered the problem with intensionality. If we keep the calculus in its own little "reducer box" 
    then all the effects outside this box are invisible to the calculus
- this means that we need to somehow bring the features we need to calculus level
- the features we need:
    * sequencing of effects (effectively statements)
    * immutable bindings
    * mutable cells
    * abstraction
    * maybe something else, like cont for exceptions, but they are not currently considered
- the way these features could be implemented + examined/generated by the triage calculus: `opcode`s
- currently, intrinsics are the way to sneak into the reduction step and intercept
    applied data to pass it to C routine; this routine has the access to state
    of the reducer and to the data at hand, it can freely inspect the data,
    reduce it in any way and just do whatever; as long as this data stays within
    the reducer (i.e. not saved to some other state), this is a fair game;
    that way we can introduce an `opcode`: specifically encoded tree node that is recognized by the `reducer`
- every feauture I've described here can be encoded
    in the calculus as-is, provided that I link stuff in a specific manner;
    this is difficult and not performant at all; hence, I would introduce opcodes to shortcut common operations
- therefore, opcodes could be used to implement different control flow, binding and more
- Sequencing `seq`; `seq :: x, y -> y_value`; Accept `x` as left child, `y` as right child; reduce `x` to value, discard it, reduce `y` to value, return `y` value
- Binds could be of three forms with respect to evaluation: `quote, let, var`;
quote is no evaluation, let is evaluate once, var is evaluate more than once;
for any term `t` that is known statically, I have its corresponding location
`t_i`; so let's introduce `set`; `set :: t_i, t -> t_value`; It reduces `t` to its value `t_value` and sets `t_i` this value; `t_i` must contain sufficient amount of space for the reference to `t_value` to be written to; using `set` at the right places with the right amount (and with the right quoting using `K x` to quote and `K x ^` to force) we can achieve all three semantics
- Abstraction is very common, hence it deserves its own intrinsic `lambda`; `lambda :: [closed-overs, mutables, parameters], body -> value`; the meaning:
    + `closed-overs` are references to the terms (or their values) that lambda has closed over on creation
    + `mutables` are the terms themselves that will change in the process of lambda reduction
    + `parameters` are the `ref`s, that will be set to `arguments` of the lambda when it will be invoked

    lambda header (left child, the list) must be copied on invocation, since `mutables` and `parameters` will be changed during invocation
    on lambda evaluation we could have several cases:
    + `mutables` is nonempty: during evaluation, the terms in the mutables array will be reduced, hence the reference to the mutable inside lambda body will see updated value
    + `closed-overs` is nonempty: need to shift indices of these closed-overs on lambda invocation
    + `parameters` is nonempty: parameters must be bound to their corresponding argument locations; then, access to a parameter inside lambda body will do two jumps: reference to parameter and then reference to the argument; Note that this is sane, since if we rewrite the parameters in the body itself, we effectively would need to copy the body, which is not ideal
- ✅ this way, the code could inspect natives and opcodes (provided there is a rule 4 or something for builtin inspection)
- update on binary encoding:
    * ✅ I need to treat the tree as a stream of bytes, with payload stored inplace; hence, I need to cleanly visualize this stream as a tree-like bytecode, which can change itself, since code is data here
    * ❌ I'm updating this list below
    * References start with `0`
    * Reference `ref` could be variable size; Lets make them:
        + `[000][5bits]` 1 byte `ref1`
        + `[001][5bits][8bits]` 2 bytes `ref2`
        + `[010][5bits][24bits]` 4 bytes `ref4`
        + `[011][5bits][56bits]` 8 bytes `ref8`

        A mutable cell is represented as a reference of maximum (8 bytes) size;
        think of it as a boxed value, that can change its size arbitrarily;
        this way, a reference to such `box` is a fearsome ref-to-ref that would allow mutability semantics
    * Tree nodes start with `11`
    * Tree0 `leaf` is just `11000000`
    * Tree1 `stem` is `11100000`, followed by any `ref`
    * Tree2 `fork` is `11010000`, followed by any `ref` and then any `node`
    * Natives/opcodes start with `10`
    * Native payload starts with `10000000` (a whole byte)
    * Native0-fixed `n0f`: `[10000000][000][5bits][48bits]` 
    * Native1-fixed `n1f`: `[10000000][001][5bits][48bits]` followed by any `ref`
    * Native2-fixed `n2f`: `[10000000][010][5bits][48bits]` followed by any `ref` unsigned int total_bitsand then any `node`
    * Native0-var `n0v`: `[10000000][100][ULEB128][payload]`, where ULEB128 is len of the payload
    * Native1-var `n1v`: `[10000000][101][ULEB128][payload]`, where ULEB128 is len of the payload; the node is followed by any `ref`
    * Native1-var `n2v`: `[10000000][110][ULEB128][payload]`, where ULEB128 is len of the payload; the node is followed by any `ref` and any `node`
    * Any opcode starts with `10` and not `10000000`
    * Sequence `seq`: `[10000001]`, it can be considered a fork
    * Set `set`: `[10000010]`, it can be considered a fork
    * Lambda `lambda` `[10000011]`, it can be considered a fork
    * Currently, there are these kinds of opcodes, but more can be added if necessary

### 04.02.2026
- So, the nodes and the encoding
- References stay the same conceptually, their encoding probably wouldn't change
    considering their count in resulting bytecode => I want as less overhead as possible here
- I've decided to make new naming and new structural changes; mainly: all nodes are now either leaf, stem or fork.
     **Every node adheres to rules 0.a and 0.b (saturation, as I've called them)**
- Next, fundamentally, there are three kinds of nodes: `delta`, `value` and `opcode`
- `value` can be split into two groups: with fixed payload and with variable one. `value` can be applied: 
    when the payload is fixed, it is assumed to be `intptr_t`, so we just reinterpret this as a function address and call it.
    When the payload is variable, there is no much (as of now) sense to apply it, so it would result in an error
- `opcode` is a hatch into the VM. Basically, a VM-native instruction, that could execute
    arbitrary code, affecting the VM/reducer state. Its length is always fixed (1 byte)
- `delta` is both a *value* and the *opcode*. Its a specific instance of two: a `value` without the payload and an opcode with special meaning. Basically, this is a way to encode triage-calculus itself into the picture of lisp-like language that operaes not on symbols, but on bytesequences/numbers.
- The whole picture looks like this:
  * the tree **is homogenous** with respect to original reduction rules. Every node can be applied to 0.a and 0.b. The behavior changes on furhter application of the fork node
  * some nodes supply payload, which can be expected via
    + intrinsics (value/opcode)
    + triage-calculus, when first converted to its representation (intrinsic `get_payload`)
  * some nodes can direct the flow of computation
    + `delta` nodes are just triage-calculus
    + `value` nodes are native calls
    + `opcode` nodes are VM-specific operations
  * every node has its interpretation:
    + when passed to intrinsics
    +  triage-calculus, when first converted to its representation (intrinsic `get_type`)
- Note the duality of `get_payload` and `get_type` above. So, basically, every node has a type.
    And every node has a payload. But oftentimes it is just `nil/false/^`
- So, any node (currently and further) has this interface with regards to its semantics:
  * leaf, stem or fork (discovered by rule 3)
  * type (discovered by intrinsic `get_type` which has stable mapping from type node to tagged value)
  * payload (discovered by intrinsic `get_payload` which provides tagged value) 
- Also, for symmetry there must be `set_type` and `set_payload` intrinsics to construct
    arbitrary native nodes out of triage-calculus encoded tagged values; for symmetry
- ⬜ revive the tagging, what is the status? And so: `get_type` returns tagged `^ int <value>`, but how `int` is encoded? And more: seems like it is a part of an ABI now.
- Chat-gpt forced me to consider this, and this is quite the point, I should enforce it here: 
  * Programs are trusted; native call payloads are forgeable; safety is the responsibility of the embedding
  * `set_type` may change the interpretation of an existing payload without validation; doing so is unsafe and may crash or call arbitrary native code
  * there is a match `payload_kind(type) ∈ {none, int, bytes, …}` and `set_payload(node, p)` is an error unless `kind(p)` matches `payload_kind(get_type(node))`

### 09.02.2026
- on encoding
- ref2 and ref8 only one remaining
- refs2 start with 00, ref8 with 01, rest of the bits are payload; 
- for ref2 is 14bit (13bit int), for ref8 62bit (61bit int)
- everything else: tag starting from 0x80 [10000000] and actual payload later
- delta0, delta1, delta2: 0x80, 0x81, 0x82
- value0, value1, value2 (fixed): 0x83, 0x84, 0x85, followed by 8bytes of signed payload
- value0, value1, value2 (variable): 0x86, 0x87, 0x88, followed by len (uleb128) and the payload as bytes
- opcode0, opcode1, opcode2: basically, three versions of the same opcode as a single byte;
    the payload should go as a children
- ✅ on gc: need to use free list in the cells

### 10.02.2026
- I've decided to use special `call` opcode to be able to call to native functions;
    therefore, any value now is not callable (but assembleble, rules 0a and 0b still work)


### 12.02.2026
- inspired by wisp, I'm pondering on the syntax of vetochka1
     (text form of vetochka0, as vetochka0 is purely a bytecode, implemented in C dsl)
- an application is just things together in the list, right associative `(a b c) == (a (b c))`
- so, there are no lists with `()`: this is an application; To encode a list: `^ a : ^ b : ^ c nil == (^ a (^ b (^ c nil)))`
- example for wisp
See [`project/wisp_syntax_example.md`](project/wisp_syntax_example.md)
---
- strings are hard... currently, I'm discussing the syntactical standpoint:
- they can be single line or multiline 
- they can contain characters that are needed to be escaped 
- if they are multiline we sometimes want to preserve whitespace, sometimes not 
- string is a series of meaningful bytes (often just utf-8), but what about other seqences of bytes?
- we sometimes want them to be null-terminated, how do we show this syntactically?
- interpolation...
- after **MANY** thoughs about strings, we do the simple stuff
- only one lexical form for strings as a sequence of *human-readable* bytes;
    single issue what we are solving here => where does the string end?
- everything else must be done by compile time functions; This means that 
    *in order to support this, we need comptime*, but this is doable, especially in triage-calculus;
    examples: `cstr, trim, dedent, bytes, hex, fmt` and others
- single lexical form is `s{}` with any number of curlies, so any prefix of `{...{` would
match ending `}...}`
- matching of curlies is greedy, so `s{{{{}}}}` is always an empty string,
    not a `{}` with three-level curlies
- beggining `s` is inseparable from `{}`, so curlies could still be used
- no escapes in the string literals are allowed
---
- ⬜ numbers are follow, ints and floats and bunch. I'd expected this to be a problem, but currently
    we are only interested in the string of digits for 64bit ints; so I need to
    make them as extensible as strings
- intrinsics, being the opcodes `lambda, seq` and others can be encoded verbatim
See [`project/vetochka_lambda_example.md`](project/vetochka_lambda_example.md)

### 13.02.2026
- WISP: https://srfi.schemers.org/srfi-119/srfi-119.html
- Curly-infix expressions: https://srfi.schemers.org/srfi-105/srfi-105.html
- I'd took all curly-infix, except for *mixed* expression, where they introduce `$nfx` to
    be able to parse infix in reader macro; Currently, I don't see any reason to include this

### 15.02.2026
- currently, final one on syntax
- first thing first: interpreter must include a module `bytecode` that
    will accept bytecode in textual format; I've called this format vetochka0
    but this is no longer necessary -- we will unite the syntax under one vetochka
- therefore, vetochka syntax (once and for all) will include (informally):
- lisp. Usual `(f a b c)` that practically means left-associative application
    `(((f a) b) c)`
- WISP: see srfi-119 above
- ❌ Curly-infix expressions: see srfi-105 above
- Literals in the form `<prefix>{contents}`, see my thoughts on the strings above;
    My current design: there are builtin literals like `s`, `i64`, floats and maybe something else; These are used to encode `value` nodes *directly*. The rest like `bytes` can be done using comptime functions. BUT! this is terra incognita for now,
    hence I will just provide the syntax convenience: any literal of the form `<prefix>{contents}` that is not builtin will be desugared to `(^ s{prefix} s{contents})`; every literal adheres to string parsing rule, meaning `i64{-1234}` is effectively `builtin.parse! s{-1234}`, so `i64{{{-1234}}}` is also valid
- Identifiers are just strings. currently, I don't see any restrictions on identifiers, except for these two:
    + they must not clash with other identifier-like symbols
    + they can't contain whitespace or control symbols like `(`, `{` and maybe `[`
- References are the way to reference terms or other single-level references (to achieve mutability);
    They are bound (declared) by the syntax: `rN::...r2::r1:(a b c)`, where `rN` to `r2`
    is an arbitrary amount of *references to reference* (`r1` in this case) and `r1` is
    a reference for the term `(a b c)`; Note that the term and reference `r1` reside in `cells` so we effectively bind references to *code*, but since code is data, this is fine; References are used by the syntax: `(:r1: a b)`, the same for double-references; References primarily used to reference distant stuff/reuse code or achieve mutability with conjunction with `.set`
- Opcodes are identifiers like `.set`, `.lambda` and a bunch. Correspond to opcodes directly
- Comments are `;{}` and `;;`; Former adheres to string parsing rules and is used to comment out
    a grouped block, latter is just to the end of the line
- Thats it. Maybe I'll need something in the future, but I don't think this will actually happen
- The thing is: this syntax is builtin. Meaning, all extension of the language will be
    done using this syntax. Currently, I think this is sufficien enough, to encode anything non-trivial in somewhat concise and pleasant to look at manner
- There is a subset of this syntax that I will call "canonical", strictly in a sense that
    it will be used as minimal syntax to represent stuff. It will contain literals desugared, won't contain comments and all applications will be explicit. This is needed for bytecode dumping and persistance
- ⬜ For future: implement this lexer+parser in the interpreter (module `bytecode`)
- ✅ For future: implement bytecode dumping

### 16.02.2026
- After reading https://srfi.schemers.org/srfi-266/ I've realized that curly-infix is very specific
    thing to add to the base language, especially since we have reflection builtin;
    So, its better to utilize it like so `expr s[1 + 2 + 3 + 4]`. Whats that? A list syntax?!?!?
- So, yeah, it is certain that I'll need some sugar for lists since writing `(^ a (^ b (^ c ^)))` or `: ^ a : ^ b : ^ c ^` is very cumbersome and lame. Since I don't want to be bothered with the details right now, let's say that `s[]` is a sugar for this kind of lists.
- Therefore, the `{}` and `[]` is free for something more important, maybe just alias it to `()` to be able to "hint meaning"? Idk, sounds interesting, but maybe it's a bad idea
- With analogy to strings: lists `s[]` can be used to express more sophisticated datastructures, like map `hashmap s[ s[s{foo1} s{bar}] s[s{foo2} s{baz}] s[s{foo3} s{qux}}]]`
- ⬜ Or maybe it is better to remove `s` prefix entirely? Then, you'll have something like
    `hashmap [ [{foo} {bar}] [{foo2} {baz}] [{foo3} {qux}] ]`. Looks more readable. Then,
    `{}` strings are **always** valid utf-8 bytes and `[]` lists are **always** nil-terminated
    proper lists. Ints like `i64` then can be encoded by designated opcodes like `.i64` that
    will take a string and parse it to real `value` node.
- Since we'll have designated build step anyway (in some way or another), these 
    opcodes can be calculated beforehand. 
- an example
See [`project/vetochka_word_count_example.md`](project/vetochka_word_count_example.md)
- ❌ not a syntax one. I need to use adjacency for apply. So, no `REDUCER_APPLY_TOKEN`.
    Therefore, reducer stack contains indices which should be evaluated next, not `f arg` implicit pairs. Therefore, **every** application must be implemented as two nearby nodes in cells, and any two nodes near each other are subject to application.
    This is done because when I was pondering on lambdas, I've realized that I have two options: apply by adjacency, or somehow structure reducer stack in a way to call a lambda that was defined way before the code I'm currently executing. This is unplausible.
    + This won't work as I don't know how to encode multistep rules 2 and 3c since their results could be sparsed
    + Reducer doesn't know anything about lambdas and such, this is another level -> problem solved

### 17.02.2026
- ⬜ There is no free cake. Meaning, I need to make sense of triage calculus first and **then** introduce new semantics,
    which means that its time to stop dreaming. All my ideas are good stuff, but they will be reevaluated under the knowledge of the calculus itself.
- I will implement the calculus part of the reducer and write tests that I've described in the old version. Then, I'll methodically, one by one, will introduce encodings for the things I need **and** maybe encode them as opcodes. This is sane, after all to use opcodes is to make interpretation faster, not to cheat.
- Syntax holds, mutability (conceptually) also holds. Sequencing, lambda and a bunch must be reimplemented in the triage calculus itself. Architecture is solid and encoding is decent. Some adjustments as new literals can be added. Bytecode handling also is decent.
- That said, I'll step away from the project again. Need to implement stuff finally, wrap it up and do my real job

### 22.02.2026
- So, conclusions for now
- after I've consulted chatgpt on the ways to keep the calculus, we came up with the new meta
- basically, keep tree calculus as is. then, introduce all the neat stuff: optimizations, side-effects, mutability, sequencing via opcodes. but opcodes themselves would be tree terms! specifically, tree values.
- this way we get full intensionality and keep all natives, just that for tree calculus the natives are just opaque deltas
- general encoding I've came up with for "opcode" term is this: `^ [tag-magic opcode stuff...]`. this is a stem because we want to be able to call this opcode somehow. general shape for an argument is simply a list `[arg1 arg2...]`. this way this term behaves as generic tree term, and when it becomes a fork, it is readdy to be called
- general evaluation strategy is now the following: given a term, reduce it using the reducer to a value; then, reduce it value using a VM and do a reduction in big-step semantics; iterate this loop so that in the end we encounter "halt" opcode and then we do stop
- sequencing is done by continuation style; lambdas can be done via combinators, with sepcific opcodes
- this whole thing is very promising and further research could help to achieve great expressivity + somewhat decent perf
- so, with this i could stall this project. efforts were very fruitful, as they helped me to understand the nature of calculus deeply and its big semantic capabilities
- Triage calculus rules:
    + `^` is a delta symbol, i.e. a binary combinator that can be also standalone
    + Important note: an apply symbol `$` is made explicit here for clarification. However, in the theory and implementation it is sufficient to
        apply nodes based on their positions only. That is, given a certain position, two consecutive nodes could be considered applied to each other if they don't form a triage calculus value (leaf, stem, fork) already, in which case the application is redundant and we can stop the computation
    + 0a. `^ $ x -> ^ x`; 
    + 0b. `^ x $ y -> ^ x y`
    + 1. `^ ^ x $ y -> x`
    + 2. `^ (^ x) y $ z -> (x z) (y z)`
    + 3a. `^ (^ w x) y $ ^ -> w`
    + 3b. `^ (^ w x) y $ (^ u) -> x u`
    + 3c. `^ (^ w x) y $ (^ u v) -> y u v` 

### 18.03.2026
- (#llm on tagging)
```
Requirements for the tagging mechanism are established:

Distinguishability — opcode term contains a magic native integer node at a fixed, known position; VM checks this position to confirm opcode identity
Reducibility — opcode term is always a fully-reduced triage value; reducer is blind to it
Saturation — VM dispatches only on fully saturated opcodes (fork applied to all args); If opcode is unsaturated, it is treated as 
a generic tree and returned verbatim
Payload carriage — opcode identity and arguments are carried as children within the fork structure
Composability — opcode term is a valid triage term, passable/storable like any other value
Shape-based inspectability — tag's tree shape (leaf/stem/fork) is sufficient for calculus-level branching via rules 3a–3c; no integer extraction needed from calculus
Fixed magic position — position of magic within the opcode tree is a fixed convention (to be designed); VM checks it in O(1) without tree walking
Validity — opcode could be invalid when it is saturated and its *internal invariants* are not met. These could include (but not limited to)
    args count mismatch, args type mismatch, invalid internal structure; In such cases VM raises an error

Unsaturated opcode: delta1 delta2 value(magic_N) delta2 payload delta0

Saturated opcode: delta2 delta2 value(magic_N) delta2 payload delta0 args

magic_N is a unique native integer per opcode, serves as opcode identity and sentinel.
Unsaturated opcode (stem) is a valid triage value, returned verbatim by VM.
User extensibility via call opcode carrying a native function pointer in payload.
Shape-based inspectability: opcode stem routes to rule 3b when used as z in rule 3, enabling calculus-level branching by shape alone.
```

### 24.03.2026
- (#runtime abi)
- Parser, VM, user programs -- all use the same ABI format to represent extended part of the core calculus
- Tagged value is of two forms:
    + `^ [tag payload...]` known as stem value. It is effectively a function, since it could be applied to another value
    + `^ [tag payload...] ^` known as fork value, which is just a tagged value
- If tagged value is a stem value, its arguments expected to be in a list `[arg1 arg2... argN]`
- Every tag is hard defined in the runtime: e.g. there are `indentifier`, `string`, `opcode.lambda` and others
- Usecases of this ABI:
    + Parser uses it to convert text to bytecode, but there are more syntactical categories than bytecode primitives:
        e.g. identifiers and strings are different, but essentially represented by the same VALUEV[N]. Hence we need tagging
    + VM uses tagging to recognize opcodes (and maybe other tagged nodes) and perform side-effects
    + User programs can rely on ABI and build programs/analyze programs in presence of extended language constructs
- Compatibility is preserved by versioning of runtime/tags, but this is a possibility, not a priority

### 02.04.2026
- ✅ Compose a proper language grammar document, with EBNF and stuff.
    Also need to think about applicability: maybe *get inspired* by the grammar and don't
    strictly follow lisp grammar at all?

### 06.04.2026
- ✅ Since I want opcodes to represent the same computation as triage calculus AND at the same
    time I want efficiency => I probably won't get both.
- My decision: build a VM that will be as efficient as possible with clear semantics, yet
    still keeping triage calculus's advantages: all three rules and homoiconicity at its maximum
- One of the approaches that can be entertained: build a VM with efficient semantics which 
    basically would have a number of opcodes and these opcodes would be just tree nodes
    and, hence, buildable by the calculus; This direction is basically what I was 
    following for these months
- And being following this path, I encountered the categorical split: VM is its own thing with big-step semantics
- Hence, VM can have state:
    * Continuation stack: to be able to "step" VM and continue the evaluation of
        current opcode if it requires the evaluation of the child opcode
    * Function frame stack or Environment: to introduce `.lambda` opcode and be able
        to reference "bound" variables
    * Maybe, other kinds of state could be explored, it all depends on the VM design
- ✅ Try to shift perspective from reducer to VM and evaluate this language's runtime
    as a VM that has tree-shaped opcodes => how would this VM be designed?
- Bound variables could be accessed by other opcode like `.var "stuff"` that would
    use VM state directly => this implies that VM can be built as a real VM with registers
    / variable stack / environment. The possibilities are large

### 28.05.2026
- long time no see
- About perspective shift: I've got new framing
- we're building new frontend for C with meta capabilities, thanks to triage calculus
- clanker wording: A reflective, staged, C-emitting language for disciplined small/medium systems programming, where project code can inspect and generate declarations, types, and implementation fragments before they lower to ordinary C and the classic C ABI
- So, this is a proper language, with proper runtime, but:
    + no *extensive* semantic analysis step
    + surface-level tailored to imperative style syntax
    + no semantics in the language itself that would leak into generated C,
        all semantics is used to do code generation
    + extensible-ish syntax to be able to encode not only C (this is mostly for the sake of experimentation)
    + keep C values: control, exlicitness, simplicity, porousness and make the language analysis-oriented
- The main framing is: we don't want to strip C down, provide new features that don't boil down to simple syntax rewrites,
    extend runtime, create "our little bouble to happily live in", invent our own stdlib, etc...
- we may provide new module system, new macro system, more expressive type system (without too much magic and being only library-side),
    convenient syntax for some cases, constant evaluation (with guarantees and corner cases), dsl support
- Basically, we don't invent zig. We're still writing C and C only. No magic, only metalanguage.
- this means that wisp syntax above isn't so applicable :((( ; I would try to incorporate algol-syntax, inspired by elixir
- Informally, hardcoded are:
```
@
()
[]
{}
.
marker:
end
,
;
prefix operator shape
infix operator shape
```
- language is statement oriented, parser only knows the shapes, no keywords beside `end` and `<stuff>:`
- main statement form: 
```elixir
expr marker1:
    stmt1;
    stmt2;
    ...
...
markerN:
    stmt1;
    stmt2;
    ...
end
```
- delimiters:
```
function calls: ,
lists:          ,
do blocks:      ;

all are trailing-optional
Semicolon omission can exist as formatter/parser convenience, but canonical syntax has ;.

(expr)          grouping only

f(a, b)         immediate function call
x (y)           FORBIDDEN
x [y]           -- call with list payload
x {y}           -- call with string payload

[expr, expr]    list
x[list]         immediate bracket index

{utf8}          UTF-8 string literal
x{utf8}         immediate string-key index / curly index

x.y             selector

They probably could be interweaved:
expr ~marker1 <expr>, ... ~markerN <expr>
expr marker1: <block> marker2: <block> ... markerN: block end 
```
- Parser recognizes infix shape, not operator meaning.
- No precedence. No mixed infix.
- annotations: `@[basically, any(expression)] expr`
- example
See [`project/vetochka_collections_vec_example.md`](project/vetochka_collections_vec_example.md);
Also [`project/vetochka_uniform.tree`](project/vetochka_uniform.tree)

### 30.05.2026
- we can do saturation: since we have `f(x,y,z) == f(x)(y)(z)`, we need to
somehow support curring for this notation to be sane. But this is a "marker" reason,
but the main one: currying is very useful
- so, how to do it: we should store an application counter in the function object
    and do actual computation as it reaches N; this way we can do "stateful" opcodes
    and fully curried functions easily


### 31.05.2026
- I need to store apply nodes in the parsed tree, since I lose application info (and
homoiconicity as a whole) when I don't do that. I don't have enough information after
control (reducer) stack is executed, so I either need to store "template" stack alongside
any referencable expression, or I need to store this info in nodes. I choose latter.
- Name resolution should happen in certain opcodes, which means that for the raw calculus
operations I would need manual name resolution
- make `~expr => :expr`
- operators are separated by whitespace, : is allowed as suffix/prefix of identifier, also `:label:` can be an empty label to support "block naming", it is equivalent to `label:^;`
- ❌ make `do: block_list? end` and `:do expression?` expressions themselves, expressions must be greedy and will only proceed on postfix
expressions, every which of them has a separate beginning token
    + nah, its better to keep them as suffixies, since we have clear separation of primary and postfix and it keeps less confusion


### 04.06.2026
- on modules: need to separate module discovery from the semantic effect on VM
- VM state: reducer, environment (immutable frames with rebinding, persistent scope graphs), underlying special forms
- ⬜ somewhere here should be ideas about box for mutability
- on special forms: it is a part of VM (but designed explicitly to be separate from VM state to be semantically swappable) that defines known symbols == opcodes; e.g. `def`, `print`, `return`, `let` and others; my thinking is that there are a couple of groups for these kinds of forms like "directives", "imperative core", "functional core" and something else
- on currying: it is good to be explicit about currying and we do that, except for `stuff: ... end` block form; idea: just make block list a separate construct
```
;; module: is a marker that becomes identical to :module, since we don't have a block form anymore
def module: Example body: do ... end
```
This way a `do a;b;c end` block is just a syntax sugar for `[do:,a,b,c]` or something
