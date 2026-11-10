# Original task records

<a id="20241220-144435"></a>

<!-- BEGIN tasks/20241220-144435/TASK.md -->
# Design the shared syntax and its lowering

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: INITIATIVE
- PARENT: 20260926-050128

- (user, spec) Source is UTF-8. Comments are `;;`, nestable `#| |#`, and `#;` for a datum. `...` continues a line.
- (user, spec) Strings are `{...}` with balanced braces and no escapes. Integers are decimal. `$` and `~` are special tokens. `do` and `end` are reserved.
- (user, 2026-08-30) Operator runs are classified by adjacency (ADR 1). Tight postfixes must be glued (R1).
- (user, spec) A newline becomes `;` in layout-active contexts after an expression-ending token.
- (user, 2026-08-30) Five application shapes: `f(args)`, `f[list]`, `f{bytes}`, `f x: v`, `f do end`. Labeled expressions are first-class.
- (user, spec) Nyads are `~[]`, `~[x]`, `~[x, y]`. Opcodes are `$ labeled_expression`. Annotations are `@[...]`.
- (user, spec) Thirteen rewrite rules lower syntax to inert terms. `{@}` marks application. Parens are erased. Mixed infix chains are left to vf.
- (user, 2026-08-30) The parser stays operator-policy-free. v0 is arbitrary syntax plus a small executable core; vf decides meaning.
- (user, 2026-07-11) There is no language-level quote.
- History: items.md.
<!-- END tasks/20241220-144435/TASK.md -->

<a id="20241225-125809"></a>

<!-- BEGIN tasks/20241225-125809/TASK.md -->
# Compact cell storage

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, spec) Cells store compact tree-shaped data and make up runtime memory. Every node is typed and has arity 0, 1 or 2.
- (user, 2026-07-11) One stable wire tag per runtime state. `NS1..NS4` and `AUX1..AUX4` are reserved. Bytes are allocated after states stabilize.
- (user, 2026-05-31) Applications are stored explicitly as apply nodes, to keep homoiconicity.
- [fact] (2026-07-10) The C parser builds a host `source_tree_t`; `bytecode_source_encode` then creates cells. The reducer is a flat loop dispatching by node arity. `OP_FN0/1/2` are an old experiment.
- Open: node types beyond nyads. See task 20260805-162238.
- History: items.md.
<!-- END tasks/20241225-125809/TASK.md -->

<a id="20241225-140425"></a>

<!-- BEGIN tasks/20241225-140425/TASK.md -->
# Lexical binders as the only nonlocality

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, 2026-08-30) Binder axiom: binders are the only nonlocality, side effects aside. Env belongs to binder nodes only; fragments are pure trees.
- (user, 2026-07-17) Model B: code stays nameful and identifiers resolve through the captured env on demand. `LOCAL` is rejected as semantics.
- (user, 2026-08-30) Executing an unbound identifier is an uncatchable trap. Observation never errors. A bound identifier is observed as its value.
- (user, 2026-08-30) `$fn` bodies are lexically scoped: they extend the declaration env.
- (user, 2026-07-09) A module is a function of its imports. `GLOBAL` is removed.
- Direction (user, 2026-08-30): `$fn` is the single universal construction form; `$f` is folded away.
- History: items.md.
<!-- END tasks/20241225-140425/TASK.md -->

<a id="20250123-091611"></a>

<!-- BEGIN tasks/20250123-091611/TASK.md -->
# General host interaction: opcodes, native bindings, opaque handles

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, spec) Opcodes are machine-native operations selected by `$`. General shape: `$ns: {namespace} op: {opcode} <params>`.
- (user, spec) Each opcode decides whether and how to evaluate each input. Opcodes are curried by application.
- (user, spec) Opcode states: `Continue(next-state, inputs)`, `Dispatch(result)`, hard trap.
- (user, 2026-07-09) Opcodes are shape-compatible node families, not delta encodings.
- ~~(user, 2026-07-11) Each transition state is its own node type. An unsaturated opcode shows as a stem under Rule 3. The saturating application dispatches at once.~~ superseded by the 2026-09-26 ruling: opcodes are designator-headed lists.
- (user, 2026-07-11) Protocol failures are uncatchable hard traps. Recoverable errors are `[{:ok}, v] | [{:error}, d]` data.
- (user, 2026-08-30) An in-flight opcode value is a code closure. Its pith shows the selected opcode and inputs as written.
- (user, 2026-08-30) Some `$` forms are inert data; executing them traps.
- Open: opcode piths, task 20260805-162238.
- History: items.md.
- Decision (user, 2026-09-26): as above, so below means opcodes must be constructible from raw forms and recognizable when looked at; recognition is part of evaluation semantics.
- Decision (user, 2026-09-26): opcodes and other special instructions are lists whose first element is a special short string, a runtime object of about 8 bytes that the evaluator recognizes quickly. To the language it is just a string. The form matches the lowered syntax: a list headed by a designator. That keeps opcodes close to their pith forms, human-readable lists; pith and storage stay separate concepts but close.
- (agent) Consistent with the encoder's `[{@}, meta, f, x]` and with 2026-09-24: tags are designator strings, interning is a hidden speedup, general values are leaves.
<!-- END tasks/20250123-091611/TASK.md -->

<a id="20260201-183604"></a>

<!-- BEGIN tasks/20260201-183604/TASK.md -->
# Development log, Dec 2024 to Jul 2026

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: log
- KIND: TASK
- PARENT: 20260926-050132

- Original development log, Dec 2024 to Jul 2026, written by the user. Entries marked "LLM" are model output.
- Epics summarize its entries by topic. Newer rulings win over it.
<!-- END tasks/20260201-183604/TASK.md -->

<a id="20260201-183605"></a>

<!-- BEGIN tasks/20260201-183605/TASK.md -->
# First cell node encoding

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, rejected
- KIND: TASK
- PARENT: 20241225-125809

- (user, 2026-01-31) Node kinds: `ref`, leaf, stem, fork, and fixed or variable natives of arity 0..2 with ULEB128 lengths.
- Rejected (user, 2026-02-01): replaced by the byte-stream encoding, task 20260201-183608.
<!-- END tasks/20260201-183605/TASK.md -->

<a id="20260201-183606"></a>

<!-- BEGIN tasks/20260201-183606/TASK.md -->
# Bytecode VM with DP register and ENV stack

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, rejected
- KIND: TASK
- PARENT: 20260529-103914

- (user, 2026-01-31) A bytecode over cells: `PUT`, `APPLY`, `DROP`, `FORCE`, `BIND`, `LOAD`, `SCOPE_ENTER/EXIT`, with a `DP` register and `ENV` stack.
- Rejected (user, 2026-02-01): effects outside the reducer are invisible to the calculus. Features moved into opcodes the reducer recognizes.
<!-- END tasks/20260201-183606/TASK.md -->

<a id="20260201-183607"></a>

<!-- BEGIN tasks/20260201-183607/TASK.md -->
# Inspect natives and opcodes from the calculus

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: opcode
- KIND: TASK
- PARENT: 20260926-050129

- Decision (user, 2026-02-01): programs can inspect natives and opcodes.
- Current form (user, spec): piths make opcodes and other runtime entities introspectable by Rule 3.
<!-- END tasks/20260201-183607/TASK.md -->

<a id="20260201-183608"></a>

<!-- BEGIN tasks/20260201-183608/TASK.md -->
# Tree as a byte stream with inline payload

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- KIND: TASK
- PARENT: 20241225-125809

- Decision (user, 2026-02-01): the tree is a byte stream with payload inline, since code is data and may change itself.
- Refs are variable size. Tree nodes start with `11`, natives and opcodes with `10`.
- Refined (user, 2026-02-09): only `ref2` and `ref8` remain.
<!-- END tasks/20260201-183608/TASK.md -->

<a id="20260209-162003"></a>

<!-- BEGIN tasks/20260209-162003/TASK.md -->
# Tag structure for node kinds

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode
- KIND: TASK
- PARENT: 20260926-050129

- (user, 2026-02-04) `get_type` returns a tagged `^ int <value>`. How `int` itself is encoded is unknown, and it is part of the ABI.
- ~~(agent) [assumption] Piths may make this moot. Not ruled.~~ rejected by the user's ruling below.
- Decision (user, 2026-09-24): this task is about the tag structure itself. Piths are only one way to inspect that structure, so they are out of scope here.
- Open: the structure of tags, including how the kind marker itself (the `int` above) is encoded.
- (user, 2026-09-24) Framing, not ruled: "Every node is typed" (spec, Cells) is about scope. Tag only really special nodes, like native integers, as rewrite rules already tag syntax nodes. The same tags serve as a convention for later user-defined datatypes, observed like TypeScript's `typeof`.
- (user, 2026-09-24) This is value metadata. Guess: a simple k-v list with fields like `tag`. Consult other dynamic languages first.
- [fact] Rewrite rules already head lowered syntax with a tag: `[{:id}, {x}]`, `[{:label}, {label}, expr]`, `[{:block}, ...]`.
- (agent, 2026-09-24) Survey from general knowledge, not checked against sources: primitive kinds live in header or pointer tag bits everywhere (Lua, Erlang, Python, JS, Lisps). User datatypes differ: Erlang records are tuples headed by an atom; Lua attaches a k-v metatable with a `__name` field; Clojure keeps a k-v metadata map apart from type and out of equality.
- Open: is `tag` one field of a value's metadata list, or separate from metadata and part of identity?
- [fact] Tree book (Jay 2021, `docs/smart/tree_book.pdf`): programs are binary trees in normal form, "already data structures", so no quotation is needed (ch. 1, 3). The only split is programs versus computations with pending applications; quotation of computations is not definable (ch. 6).
- [fact] Tree book 5.4: tagging is `tag{t, f} x = f x` and `getTag(tag{t, f}) = t`. A tag doesn't change behaviour but is recoverable by inspection, for comments, names or types. It is a convention over plain trees: nothing marks a tree as tagged.
- [fact] Vetochka records: no language-level quote (2026-07-11); "everything in the language is data, executable constructs included" (user, 2026-08-30); application exists only in machine control, while data carries `{@}` markers (user, 2026-08-30).
- Deferred (user, 2026-09-24): value identity is a runtime topic, not discussed now.
- [fact] R7RS (`docs/smart/r7rs.pdf`, 1.1, 3.2, 5.5): types are associated with values; built-in types are disjoint and known only through predicates such as `pair?` and `number?`; `define-record-type` is generative, each use makes a new type distinct from all others. The standard has no way to read a value's type or attach metadata.
- Proposal (user, 2026-09-24): values follow the existing head convention, `[{:value}, [meta...], ...]`. What counts as a value is the user's decision; numbers and strings are values, other things are already marked by their head.
- (agent) Challenge: closures and in-flight opcodes are values by the 2026-08-30 rulings, and their env is hidden, so they can't be plain lists. Primitives such as integers sit below the convention and still need their own kind tag.
- (agent) Challenge: a convention over lists is forgeable. Any list headed by `{:value}` looks like a value, the same weakness as `getTag` in the tree book.
- (agent) Challenge: a generic `{:value}` head says nothing about the kind, so the kind moves into `meta`. A kind-named head such as `{:int}` would carry it directly, like `{:id}` and `{:label}` do.
- (agent) For the proposal: it has the same shape as the ruled metadata list for lowered nodes, `[{node}, [version, line, file, ...], ...payload]`, and stored metadata keeps the hermetic principle.
- (user, 2026-09-24) Replies: closures can carry a head like `{:closure}` too. A tag can be an interned string (future). Forgery is accepted: a forged value is yours to use. `{:value}` is just a category; reading `meta` second doesn't change intent. Syntax and user data unify via metadata.
- Open (user, 2026-09-24): which kinds get a first-class head (like `{:closure}`) and which go under a category. An optimization question, like core opcode shorthand versus `$ns: {namespace} op: {opcode}`.
- Decision (user, 2026-09-24): interning tags is only a hidden speedup.
- Decision (user, 2026-09-24): a string starting with `:` in head position is a tag designator. Storage may omit stored string tags as tree compression; that concern affects only Rule 3.
- Decision (user, 2026-09-24): built-in kinds have their own tags, and they carry `meta` too.
- ~~(agent) you can't forge a closure~~ corrected (user, 2026-09-24): a closure is forged the usual way, by definition and application. In principle anything can be forged; not everything by composing lists.
- Follow-up: first-class heads are an optimization question, task 20260924-105828.
- (user, 2026-09-24) This is a subtyping relation: tagged nodes refine the basic structures.
- (agent) Challenge: the subtyping is structural only. A tagged node is a list, so list operations see its tag and `meta` as ordinary elements. Jay's tags are invisible to application; these are not.
- (agent) [fact] The spec has `integer_literal` in the grammar but no rewrite rule for literals, so neither integers nor strings have a lowered shape yet.
- Decision (user, 2026-09-24): the list leak is accepted. Skipping heads would be wanted, but Jay's tags hide themselves only from application, not from structure, so this is not solved for lists as such.
- (agent) Reading, not ruled: integers are basic because an integer's structure is its meaning. Bytes are not: the rewrite rules use the same byte-string node as tag designator `{:id}`, name `{x}`, operator `{op}` and literal `{bytes}`, so the bytes alone don't say what they mean.
- Decision (user, 2026-09-24): Vetochka follows triage: only structure is operated on. Some nodes must still be opaque real-world handles: primitives, and other values and functions.
- Decision (user, 2026-09-24): opaque nodes are distinguished structurally by the tag first in their list. Integers get `{:i64}` too, regardless of provenance.
- (user, 2026-09-24) A list is recognized by structure; a proper list ends with `~[]`.
- (agent) Challenge: the tag `{:i64}` is a byte string, and byte strings are opaque and tagged too. Observing a tag as a tagged list recurses. Something must bottom out in pure structure, for example the tree book's natural numbers and char lists (the July bytes-lens candidate).
- (agent) Challenge: by the spec's encoding a list cell is `~[x, rest]`, so its left child is the element, not a leaf. Any fork looks like a list cell; only walking to `~[]` confirms a proper list.
- Decision (user, 2026-09-24): general values such as i64 and strings are leaves. They don't recurse on themselves.
- (agent) Challenge: Rule 3 sees every leaf as the same `~[]`. If opaque values observe as leaves, `{:i64}` and `{:bytes}` look identical to Rule 3, so comparing tags needs a native operation.
- [fact] R7RS 6.4: a list is the empty list or a pair whose cdr is a list; `list?` walks to check it and returns false for improper and circular lists. There is no runtime list type separate from pairs.
- Decision (user, 2026-09-24): native equality for tags is inevitable, since Vetochka has certain kinds of values.
- [fact] Tree book 3.7, 4.7, 5.5: Jay has no native ints or strings. A number n is `K^n` applied to a leaf; bits are 0 and 1; bytes are eight-tuples of bits; strings are lists of bytes. So 0, nil and the empty string are all the same leaf, and nothing tells them apart. Types are separate trees tagged onto terms, e.g. a leaf applied to the string "Nat", compared structurally.
- Decision (user, 2026-09-24): a `list?` predicate that walks, as in Scheme. Treat everything as plain trees for now.
- (user, 2026-09-24) The subtype lattice bottoms out in the basic shapes such as leaf.
- [fact] Tree book 5.4: `tag{t, f} = d{t}(d{f}(KK))`; applying it reduces through `K (f x) (t x)` to `f x`, so the tag is dropped on application. Only programs (normal forms) are tagged. Tagged fixpoints need the tag inside (`Y2t`). Triage's rule 2 places arguments differently, so the construction would need re-deriving.
- Which nodes carry meta, and why: see 20260903-085028.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
- Direction (user, 2026-10-19): explore integrating Jay's `tag_apply` and `getTag_tag` laws. The user considers them useful but has not selected an integration design.
- Continuation (agent, 2026-10-19): start from the tagging facts above: application preserves the wrapped program's behavior, while inspection recovers the annotation. Re-derive a candidate under Vetochka's triage rules before treating Jay's construction as applicable.
- Open (agent, 2026-10-19): distinguish a transparent annotation from the existing head designators that select runtime behavior. The versioning requirement in 20260805-162238 makes this distinction relevant.
- Boundary (agent, 2026-10-19): `tag_apply` does not imply that changing a runtime opcode or semantic-version selector preserves behavior. The proposed integration must specify which tag role the law covers.
- Source for continuation: https://raw.githubusercontent.com/barry-jay-personal/tree-calculus/master/Intensional_Programs.v (`tag_apply`, `getTag_tag`).
<!-- END tasks/20260209-162003/TASK.md -->

<a id="20260209-185016"></a>

<!-- BEGIN tasks/20260209-185016/TASK.md -->
# Free list in cells for gc

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- KIND: TASK
- PARENT: 20241225-125809

- Done (user, 2026-02-09): garbage collection uses a free list in cells.
<!-- END tasks/20260209-185016/TASK.md -->

<a id="20260212-163430"></a>

<!-- BEGIN tasks/20260212-163430/TASK.md -->
# Extensible number literals

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-02-12) Number literals should be as extensible as strings. Only 64-bit integers are needed now.
- [fact] The spec has only decimal integer literals.
<!-- END tasks/20260212-163430/TASK.md -->

<a id="20260215-122341"></a>

<!-- BEGIN tasks/20260215-122341/TASK.md -->
# Lexer and parser for textual bytecode in the interpreter

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, reducer
- KIND: TASK
- PARENT: 20260529-103914

- (user, 2026-02-15) The interpreter should have a `bytecode` module that reads the textual syntax.
- [fact] reducer/ now has a C parser and a source-to-cells encoder.
- Done (user, 2026-09-24): the C parser and source-to-cells encoder close this task.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260215-122341/TASK.md -->

<a id="20260215-122342"></a>

<!-- BEGIN tasks/20260215-122342/TASK.md -->
# Bytecode dumping

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- KIND: TASK
- PARENT: 20241225-125809

- Done (user, 2026-02-15): bytecode dumping, using a canonical syntax subset: literals desugared, no comments, explicit applications.
<!-- END tasks/20260215-122342/TASK.md -->

<a id="20260215-122343"></a>

<!-- BEGIN tasks/20260215-122343/TASK.md -->
# Curly-infix expressions

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, rejected
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-02-13) Adopted SRFI-105 curly-infix, minus mixed expressions.
- Rejected (user, 2026-02-16): after SRFI-266, curly-infix is too specific for the base language. Reflection covers it.
<!-- END tasks/20260215-122343/TASK.md -->

<a id="20260216-100019"></a>

<!-- BEGIN tasks/20260216-100019/TASK.md -->
# Drop the s prefix from literals

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-02-16) Proposed dropping the `s` prefix: `{}` always UTF-8 bytes, `[]` always proper lists.
- Done: the spec has bare `{...}` strings and `[...]` lists.
<!-- END tasks/20260216-100019/TASK.md -->

<a id="20260216-130542"></a>

<!-- BEGIN tasks/20260216-130542/TASK.md -->
# Application by cell adjacency

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding, rejected
- KIND: TASK
- PARENT: 20241225-125809

- (user, 2026-02-16) Two adjacent nodes in cells apply to each other, so the reduce stack needs no apply token.
- Rejected (user): results of rules 2 and 3c can be scattered, so adjacency can't encode them.
- Later (user, 2026-05-31): applications are stored as explicit nodes.
<!-- END tasks/20260216-130542/TASK.md -->

<a id="20260217-074816"></a>

<!-- BEGIN tasks/20260217-074816/TASK.md -->
# Understand the triage calculus before adding semantics

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- KIND: TASK
- PARENT: 20260529-103914

- Decision (user, 2026-02-17): implement and test the calculus part of the reducer first. Then add encodings one by one, with opcodes only as speedups.
- [fact] (2026-06-13) The reducer implements calculus application, with tests.
- Done (user, 2026-09-24): "as above, so below".
<!-- END tasks/20260217-074816/TASK.md -->

<a id="20260405-181501"></a>

<!-- BEGIN tasks/20260405-181501/TASK.md -->
# Grammar document with EBNF

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- Done (user, 2026-04-02): a grammar document with EBNF.
- Current form: `docs/new_spec/02_syntax.md`.
<!-- END tasks/20260405-181501/TASK.md -->

<a id="20260529-103914"></a>

<!-- BEGIN tasks/20260529-103914/TASK.md -->
# v0 machine: CESK semantics

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine
- KIND: INITIATIVE
- PARENT: 20260926-050130

- Decision (user, 2026-08-30): core evaluation is triage rules 0..3 exactly as in the spec, plus identifier handling. Branch-first, no implicit forcing.
- (user, 2026-08-30) Timing comes from control position only. Rule 2 copies raw operands; duplicated effects are accepted.
- (user, 2026-08-30) Rule 3 never evaluates `{@}` data. ~~A pending machine application in the operand is evaluated to expose its root shape.~~ superseded by the 2026-09-25 ruling: Rule 3 never evaluates.
- (user, 2026-08-30) No specialize stage: opcodes apply until no applications remain. The machine runs `exec` on a value and doesn't walk `{@}` on its own.
- (user, 2026-07-09) The machine owns all application dispatch, family first and arity second.
- (agent, 2026-08-30) The July rule catalogue in `docs/spec/03_v0.md` is a usable CESK seed. `SHAPE-APPLY` survives only for pending control; `RESTORE-DELTA` is dead.
- Open: ZINC-style argument handling and the minimal machine state. See archive/20260613-120000/minimal-machine.md.
- History: items.md.
- Decision (user, 2026-09-25): Rule 3 never evaluates. Applications persist and Rule 3 observes them like any node (arity 2); identifier dereference is its only indirection, which is how programs use binders' non-locality with Rule 3. To inspect a result rather than the computation, bind it with `$fn` and inspect the identifier. This departs from treecalcul.us, where an application cannot reach a Rule 3 branch, and the spec should say so.
- Plan to a working v0 (user+agent, 2026-09-26): 2026-09-26.md.
<!-- END tasks/20260529-103914/TASK.md -->

<a id="20260529-103915"></a>

<!-- BEGIN tasks/20260529-103915/TASK.md -->
# C gains a modern frontend with compile-time meta; output stays ordinary C

- STATUS: OPEN
- PRIORITY: 100
- TAGS: framing
- KIND: INTENT

- (user, 2026-05-28) A reflective, staged, C-emitting language. No language semantics leak into the generated C.
- (user) Porosity is the motivation: stay porous to other systems like C, and reflectively porous inside. See porosity.md.
- (user, 2026-08-30) Stack: v0 machine, then vmeta, then a vsystem compiler written in vmeta that emits ordinary C.
- (user, 2026-08-30) Internal porosity exists at build time only; the emitted C carries external porosity. vsystem has no runtime reflection.
- (user, 2026-08-30) An appliance is a language plus mandated passes. Disciplines like ownership become analysis libraries.
- (user, 2026-08-30) The substrate is permissive; appliances restrict at their own compile step.
<!-- END tasks/20260529-103915/TASK.md -->

<a id="20260529-103916"></a>

<!-- BEGIN tasks/20260529-103916/TASK.md -->
# Opcode efficiency versus triage fidelity

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Decision (user, 2026-04-06): efficiency and exact triage computation can't both be had.
- Build an efficient VM with clear semantics that keeps triage's rules and homoiconicity. Opcodes are tree nodes the calculus can build.
<!-- END tasks/20260529-103916/TASK.md -->

<a id="20260529-103917"></a>

<!-- BEGIN tasks/20260529-103917/TASK.md -->
# Shift perspective from reducer to VM

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Decision (user, 2026-04-06): treat the runtime as a VM with tree-shaped opcodes and big-step semantics.
- VM state may include a continuation stack and an environment for bound variables.
<!-- END tasks/20260529-103917/TASK.md -->

<a id="20260601-084008"></a>

<!-- BEGIN tasks/20260601-084008/TASK.md -->
# do-blocks as standalone expressions

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, rejected
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-05-31) Proposed `do: block_list? end` and `:do expression?` as standalone greedy expressions.
- Rejected (user): keep them as suffixes, for a clear primary and postfix split.
- Later (user, 2026-08-30): bare `do ... end` became a standalone entry lowering to `[{:block}, ...]`.
<!-- END tasks/20260601-084008/TASK.md -->

<a id="20260604-111747"></a>

<!-- BEGIN tasks/20260604-111747/TASK.md -->
# Boxes for mutability

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders
- KIND: TASK
- PARENT: 20241225-140425

- (user, 2026-06-04) Mutability should come through boxes. Ideas are pending.
- (user, 2026-02-01) Early idea: a mutable cell is a max-size ref, so a ref-to-ref gives mutability.
- (LLM, 2026-06-15) Cells may back mutable boxes; immutable execution is the CEK subset.
- [fact] (2026-07-10) The minimal machine draft has no store or boxes.
<!-- END tasks/20260604-111747/TASK.md -->

<a id="20260606-141936"></a>

<!-- BEGIN tasks/20260606-141936/TASK.md -->
# Newline-based semicolon insertion

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- Done (user, 2026-06-05): newlines replace most `;`, and `...` continues a line.
- Current rule (spec): a newline becomes `;` when the context is layout-active, `...` doesn't suppress it, and the previous token ends an expression.
<!-- END tasks/20260606-141936/TASK.md -->

<a id="20260606-141937"></a>

<!-- BEGIN tasks/20260606-141937/TASK.md -->
# Compile before execution

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- (user, 2026-06-05) Code is compiled before execution: parse, then name and module resolution and opcode generation.
- ~~Superseded~~ (user, 2026-08-30): there is no specialize stage.
<!-- END tasks/20260606-141937/TASK.md -->

<a id="20260606-141938"></a>

<!-- BEGIN tasks/20260606-141938/TASK.md -->
# Migrate implementation to C plus Lua

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: project
- KIND: TASK
- PARENT: 20260926-050130

- (user, 2026-06-06) Idea: move the implementation from zig to C plus Lua, dropping zig.
- [fact] The build still uses `build.zig`. No decision recorded.
- Reference (user, 2026-09-24): take the Lua plus C build scripts from https://github.com/UberDever/muh-game.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
- Decision (user, 2026-09-26): migrate before the machine core slice (20260926-091320). Extract and vendor the muh-game build; how is open.
- [fact] (agent, 2026-09-26) muh-game at d93f303: `scripts/build.lua` (836 lines), `build_impl.lua` (361), `build_test.lua` (621), a per-target manifest (`manifest.debug.linux.lua`, 225). Needs host `lua` and the `lfs` C module (`vendor/lfs`); vendor libs build through cmake. Host has lua 5.4, ninja, clang.
- Open (agent): copy the scripts into this repo, or extract a standalone build repo vendored by both; what replaces the Zig unit tests (25 tests in `reducer/test_*.zig`).
- Decision (user, 2026-09-26): option B. Extract the build into a standalone `muh-build` project. Top level `muh-build/scripts/` holds `build.lua` with vendored Lua and `lfs`. Flow: compile host Lua with a general clang or gcc, compile `lfs`, then run `lua build.lua` with a manifest.
- Decision (user, 2026-09-26): muh-build's integration test suite is a small project under `muh-build/tests/muh-game/`, with its manifest and build test there. It copies muh-game's layout verbatim (`cmd`, `internal` for internal packages, `vendor`) and recreates the essential bits: vendoring libraries for sure, packages maybe. muh-game's `internal/functest` is a general-tests package, not essential.
- Direction (user, 2026-09-26): vetochka tests everything in Lua, since its deployment is library-first. Recalled by the user; check against history first.
- [fact] (agent) Test history from git: Python tests (2024-12, `a69d92d`); Python over a C eval module (2025-01, `ac7528d`); "asan is such a pain" (2025-04-21, `bc8d32b`); "Discarding python for test language, using pure C" (2025-04-28, `7fd314d`); Zig tests "to ease the pain of C integration" (2025-09-21, `a1fd1a4`, reason as recalled by the user). No commit records the Lua-for-tests reasoning; `tasks/20260201-183604/project_history.md` only links this task.
- Correction (user, 2026-09-26): Python was dropped to keep the project hermetic and self-sufficient, in the spirit of https://github.com/tsoding/nob, not because of ASan.
- Decision (user, 2026-09-26): `~/dev/c` is an implicit monorepo. Projects are siblings and find each other by location; nothing manages the whole tree. Example: https://github.com/UberDever/std.any_ref builds with `-I..` and includes `"std.any_ref/third_party/nob.h/nob.h"`. muh-build is expected to supersede https://github.com/UberDever/std.gob; leave gob unchanged for now.
- Decision (user, 2026-09-26): Linux only for now. muh-build's integration suite vendors SDL3 as muh-game does; unit tests later, if any.
- Decision (user, 2026-09-26): vetochka ships its Lua bindings, as a `public/` package, alongside public C packages as in std.gob and std.any_ref. It adopts the `cmd`, `internal`, `vendor` layout. The Lua test runner is hand-rolled. muh-build has no `tasks/` for now.
- [fact] (agent, 2026-09-26) Lua plus lfs need neither Lua's makefile nor `liblua.a`: `lua.c` leaves the hook `luai_openlibs` open (`#if !defined(luai_openlibs)`, line 695 in 5.5.0). One C file forward-declares `struct lua_State` and a `host_openlibs`, defines `luai_openlibs(L)` to it, includes `onelua.c` and `lfs.c`, then defines `host_openlibs` as `luaL_openselectedlibs(L, ~0, 0)` plus `luaL_requiref(L, "lfs", luaopen_lfs, 0)`. Built with one `cc -std=c99 -O2 -DLUA_USE_LINUX ... -lm -ldl`, and with clang ASan+UBSan: no warnings; `require "lfs"` works. Including `lua.h` before `onelua.c` breaks its POSIX feature macros. Experiment used muh-game's vendored Lua 5.5.0 and lfs 1.9.0.
- (agent) The same file shape makes vetochka's test runner: preload the vetochka Lua binding next to lfs and link vetochka's objects, all built with one set of flags, sanitizers included.
- Decision (user, 2026-09-26): the project root is the manifest's directory (option A); no optional settings for now. The fixture keeps only what exercises the build system. Vendored Lua is stripped to its sources, with the host file inside it. No initial muh-build commit before vetochka integrates it.
- [fact] (agent, 2026-09-26) `~/dev/c/muh-build` (uncommitted): `scripts/build.lua`, `build_impl.lua`; `vendor/lua-5.5.0` (release `src/` minus `luac.c` and makefile, plus `host.c`); `vendor/lfs-1.9.0` (`lfs.c`, `lfs.h`, LICENSE); `tests/muh-game/` (libs `game`, `ecs`; tests-only `functional`; `cmd:game`; SDL3). Bootstrap: `cc -std=c99 -O2 -DLUA_USE_LINUX -Ivendor/lua-5.5.0 -o scripts/lua vendor/lua-5.5.0/host.c -lm -ldl`. `scripts/lua tests/muh-game/build_test.lua`: 126/126 pass.
- [fact] (agent) Fixed on the way, both present in muh-game d93f303: the test expected package `functional` while the directory was `functest`; the test expected log tag `wrote` while the code logged `compile_commands`.
- (agent, 2026-09-27) The monorepo build system moved to its own initiative in muh-build: `~/dev/c/muh-build/tasks/20260927-074554` (dialogue ledger, experiment, fixes 1 to 6, module API, uniform includes). All uncommitted, awaiting the user's review before vetochka's integration.
- Proposed integration steps (agent, 2026-09-27), not yet agreed: 1 layout move to `internal/<pkg>/` with `"vetochka/internal/<pkg>/api.h"` includes and a root manifest, Zig kept green; 2 vendored Lua as a vendor lib; 3 Lua binding, runner cmd, tests ported; 4 remove Zig; 5 commits. Open: test access to internals (one binding, or public binding plus test-only module); Lua test location; keeping Zig during the port.
- (agent, 2026-09-29) muh-build is committed and public (https://github.com/UberDever/muh-build, CI green on Linux, macOS, Windows); its model changed since the steps above: recipes with declared requirements, vendored libraries as sibling repos with manifests, shared builds in `<workspace>/build/`, stock Lua.
- Decisions (user, 2026-09-29) for the integration: packages with an `api.h` become `public/` (keep it simple, all transient); Lua tests go in a top-level `tests/`; Zig keeps running until the tests are ported; vendored Lua and the header-only libraries become sibling repos in the workspace `~/dev/vetochka-repo`, which holds views (symlinks) of the repos in `~/dev/c`, the main tree, for opening in an editor.
- [fact] (agent, 2026-09-29) Step 1 done: `reducer/` split into `public/{allocator,bytecode,cells,domain,reducer,source}/` (every package with an `api.h`) and `internal/headeronly/` (the stb_ds and arena implementation units), Zig tests in `tests/zig/`; includes name the repo (`"vetochka/public/cells/api.h"`); `manifest.linux.lua` (modes debug with ASan+UBSan, the default, and release; `packages` in link order reducer, bytecode, cells, source, allocator, domain, headeronly, found from the objects' symbols) and `recipe.lua` (requires `stb_ds-0.67`, `nob_da-3.8.2`, `arena`). The headers moved to those sibling repos in `~/dev/vetochka-repo`, each with a header-only manifest. muh-build builds all seven archives through the view `~/dev/vetochka-repo/vetochka`; a second build does nothing. `zig build test-all`: 25/25, plain and sanitized. `PROJECT_ROOT` and `PATH_SEP`, passed by Zig but used nowhere, are dropped.
- Open (agent, 2026-09-29): the vendored headers are now in no repository: neither `~/dev/vetochka-repo` nor `~/dev/c` is a git repo. vetochka's GitHub CI (`zig-test.yml`) checks out vetochka alone, so it will fail on the next push. Needs a decision on where vendored repos are versioned and how CI assembles the workspace.
- Decision (user, 2026-09-29): the workspace `~/dev/vetochka-repo` becomes a git repo holding the vendored libraries; vetochka and muh-build stay repos of their own, placed next to them (local symlinks, ignored). No submodules (agent's argument: a second clone of vetochka would drift from `~/dev/c/vetochka`, and pins fight CI testing the current commit); no `repos.lua` (nothing would read it). Reproducibility is kept manually: the workspace README records which commits of the three repos work together.
- [fact] (agent, 2026-09-29) Workspace repo committed (`bf0481a`, not pushed; its GitHub repo does not exist yet). vetochka `308a648`: `build.zig` takes `-Dworkspace`; CI checks out the workspace repo with vetochka (and, for a new muh-build job, muh-build) inside it. A local simulation from fresh clones passes all three jobs: Zig 25/25 plain and sanitized, muh-build builds the seven archives.
- Decision (user, 2026-09-29): a central repo of vendored libraries, `mine` (the workspace repo renamed on GitHub: https://github.com/UberDever/mine), one directory per library with its manifest. The workflow is manual: a project's recipe lists what it needs and its README gives the commands to check those entries out next to it (a sparse checkout of `mine`), with muh-build cloned alongside; then everything builds as usual. `mine` ignores only `/build/`; projects placed inside a checkout are excluded per checkout (`.git/info/exclude`).
- [fact] (agent, 2026-09-29) Following the README's commands in a fresh directory: sparse checkout clean, muh-build builds the seven archives, Zig 25/25 with `-Dworkspace=..`.
- (user, 2026-09-29) Do not duplicate the same information across files. (agent) The libraries vetochka needs are named only in recipe.lua (the README points there, CI checks out all of mine, build.zig adds every workspace directory); how to check out mine only in mine's README; how to get a Lua only in muh-build's README (CI installs the distribution's Lua 5.4).
- [fact] (agent, 2026-09-29) Step 2 done: Lua 5.5.1 is an entry of mine (release sources unmodified; its manifest's preconfigure compiles all but lua.c into liblua.a and exports the headers, the archive and `-lm -ldl`). vetochka: `public/binding/` (renamed `public/lua-binding/` by the user, 2026-09-29) (`luaopen_vetochka`: `parse` to an s-expression, `format` to canonical text; named `binding`, not `lua`, so its archive does not collide with Lua's `liblua.a`), `cmd/vtest` (runs a Lua script with `vetochka` preloaded; exit code from the script), `tests/run.lua` (hand-rolled harness) and `tests/source_test.lua`: 3/3 under ASan and UBSan. Zig still 25/25.
- ~~Open (agent, 2026-09-29): canonical formatting is not idempotent: `format("f(x,   y)")` is `"f(x, y)\n"`, and formatting that gives `"f(x, y);\n"` (the trailing newline comes back as `;`). The Zig tests claim only that canonical form parses again (`tests/zig/test_source.zig:169`), so the Lua test checks that. Whether canonical form should be a fixed point is the user's call.~~ moved to its own task 20260929-125822 (user asked to record it, 2026-09-29).
- [fact] (agent, 2026-09-29) Step 3 done: all 25 Zig tests ported to Lua (`tests/source_test.lua` 3, `tests/bytecode_test.lua` 16, `tests/reducer_test.lua` 6), 25/25 under ASan and UBSan; flipping one expectation per file fails exactly those tests. The binding mirrors the C API one to one (transient, for tests): `vetochka.cells` (types from the same X-macro as the C enum, projections, `new_*` constructors, `set_arity`, `create` giving a cells object with alloc, write, header, get, free, span, deref, left, right, build (the tree builder), reducer), the reducer object (push, step, result, has_result, reset, error), and `encode(text, {source_meta})`. Nodes are Lua tables `{type, size, ref | f | v}`. Two binding bugs found by the tests and fixed: node tables read through a relative stack index, and a tree builder leaked when `build` raised an error (it now lives in a userdata with `__gc`). The Zig tests still pass 25/25 until step 4 removes them.
- (user, 2026-09-29) Run locally; do not push (the agent cannot without the user).
- [fact] (agent, 2026-09-29) Step 4 done: Zig removed (`build.zig`, `tests/zig/`, `.zig-cache`, `zig-out`, their `.gitignore` lines); CI is one job, `tests.yml`: build with muh-build and run the Lua tests, in debug mode (ASan, UBSan). `.clangd` and `.clang-format`, left in `reducer/` by step 1, moved to the root. vetochka builds and tests with muh-build and Lua only.
- Closed (user asked, 2026-09-29): done. vetochka builds with muh-build in a workspace of mine's entries (Lua included), its 25 tests are Lua, Zig is removed (4b0ff4c); verified from fresh clones of the three repos, and in CI before the Zig removal (run 36571553346: Lua 25/25, Zig 25/25).
<!-- END tasks/20260606-141938/TASK.md -->

<a id="20260606-141939"></a>

<!-- BEGIN tasks/20260606-141939/TASK.md -->
# Function arity: positional and labeled arguments

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: binders
- KIND: TASK
- PARENT: 20241225-140425

- (user, 2026-06-06) Options: positional plus labeled arguments, or only positionals count.
- Decision (user, 2026-06-11): no variadics; named parameters count as parameters; strict signatures.
- Current form (spec): `$fn: [params]` has arity N; arguments come through `to:`.
<!-- END tasks/20260606-141939/TASK.md -->

<a id="20260613-151555"></a>

<!-- BEGIN tasks/20260613-151555/TASK.md -->
# Semantic boundary for specialized IR

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- (user, 2026-06-06) A clear semantic boundary for specialized IR was needed.
- Answered by Layer 3 specialization (2026-07-11). ~~Superseded~~ (user, 2026-08-30): no specialize stage.
<!-- END tasks/20260613-151555/TASK.md -->

<a id="20260613-151556"></a>

<!-- BEGIN tasks/20260613-151556/TASK.md -->
# Host rim and bootstrap

- STATUS: OPEN
- PRIORITY: 100
- TAGS: host
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, 2026-06-11) The executable launches bootstrap v0 code that loads sources and prepares them for execution.
- (user, 2026-07-11) `load_file` is the only required host effect. `parse_term` is pure and returns inert term data for the full shared syntax.
- (user, 2026-08-30) The host API is modeled on WASI: print, load and trap analogues. `parse_term` stays native by choice.
- (user, 2026-08-30) Load, parse, transform on the machine, then feed the root term. `exec` is the host-side door.
- ~~(2026-07-09) a `{specialize}` backbone intrinsic~~ superseded 2026-07-11 and 2026-08-30.
- (user, 2026-07-09) No capability machinery in v0. The June idea of capabilities in `initial_term` remains a direction.
<!-- END tasks/20260613-151556/TASK.md -->

<a id="20260614-121150"></a>

<!-- BEGIN tasks/20260614-121150/TASK.md -->
# Revisit byte encoding

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: encoding
- KIND: TASK
- PARENT: 20241225-125809

- Done (user, 2026-06-14): byte encoding revisited. No record of the outcome beyond the log mark.
<!-- END tasks/20260614-121150/TASK.md -->

<a id="20260614-140231"></a>

<!-- BEGIN tasks/20260614-140231/TASK.md -->
# Abstract opcode node creation in the source encoder

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: reducer
- KIND: TASK
- PARENT: 20260529-103914

- [fact] `new_op_fn` in `reducer/bytecode_source.c` hardcodes `OP_FN0`. The comment asks to derive it from VM info.
- [fact] `OP_FN0/1/2` are an old experiment, not target semantics (2026-07-10).
- Closed (user, 2026-09-24): the `{fn}` / OP_FN0 recognition was dropped from the encoder; `$fn:` superseded it.
<!-- END tasks/20260614-140231/TASK.md -->

<a id="20260711-131255"></a>

<!-- BEGIN tasks/20260711-131255/TASK.md -->
# v0 layers restoration

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process
- KIND: TASK
- PARENT: 20260926-050132

- (user+agent, 2026-07-09..11) Restored Layers 0..4 into archive/20260711-120000/v0_layers.md. Decisions are in archive/20260711-120000/2026-07-11.md.
- (agent, 2026-08-05) archive/20260711-120000/v0-state-audit.md separated evidence from rulings.
- (user, 2026-08-30) archive/20260711-120000/v0_layers.md is obsolete where it contradicts the 2026-08-30 rulings. Layer 3 specialization and `{specialize}` are dead.
- Layer 5 continues in task 20260711-131257.
<!-- END tasks/20260711-131255/TASK.md -->

<a id="20260711-131256"></a>

<!-- BEGIN tasks/20260711-131256/TASK.md -->
# Recursion derived, not primitive

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: binders
- KIND: TASK
- PARENT: 20241225-140425

- Decision (user, 2026-07-09): recursion is derived, not primitive. `{rec}` is rejected.
- `fix` is the plain Y combinator through call-by-name `{form}`. Mutual recursion passes a function list through one `rec`. Verified in a stand-in.
- (user, 2026-08-30) `let`'s `<~` clause routes through `$form` and fix.
<!-- END tasks/20260711-131256/TASK.md -->

<a id="20260711-131257"></a>

<!-- BEGIN tasks/20260711-131257/TASK.md -->
# Write the v0 CESK rules

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, spec
- KIND: TASK
- PARENT: 20260529-103914

- The spec's v0 CESK section has notation only.
- Inputs: the machine epic's rulings, binders (task 20260830-165615), the July catalogue in `docs/spec/03_v0.md`, and archive/20260613-120000/minimal-machine.md.
- Obligations: proper tail calls, a frame invariant, and native trampolines with no machine re-entry from C.
- ~~Open (agent, 2026-09-25): evaluation order decides whether the Rule 3 exception for pending applications (2026-08-30) can ever trigger: branch-first evaluates the operand before Rule 3; callee-first (July `RUN-APPLY`) retains it.~~ dissolved by the 2026-09-25 ruling below.
- Rule 3 never evaluates (user, 2026-09-25): see 20260529-103914.
- Reminder for the user (2026-09-25): state in the spec (triage calculus section of `03_v0.md`) that Vetochka departs from treecalcul.us: applications persist and Rule 3 observes them instead of evaluating them first.
- (user, 2026-09-25) Consequence of the Rule 3 ruling: inspecting the result of a triage inside another triage takes explicit binding, since Rule 3 sees the inner application, not its result.
- ~~(agent, 2026-09-25) Scope of that cost: `$fn` functions evaluate their arguments (spec, `to:` step; plain application eager, 2026-08-30), and execution continues until a value, so one `let[r = t1(v)] do t2(r) end` suffices per inspected result. Nyads take raw operands (rules 0a, 0b, 2), so values built by applying nyads can hold unevaluated applications in their branches; fully computing such a tree needs a deep force: a Rule 3 walk that binds each branch, likely a library routine. A language-level `eval` stays excluded (2026-07-11; `exec` is host-only).~~ corrected (user, 2026-09-25): nothing requires an application-free tree. A value whose branches hold applications is finished; those applications are data. A consumer binds a branch only when it wants that result. No recursive evaluation follows from the semantics; the "deep force" assumed Jay's normal forms.
- `exec` is one step (user, 2026-09-26): see 20260903-085029.
<!-- END tasks/20260711-131257/TASK.md -->

<a id="20260711-131258"></a>

<!-- BEGIN tasks/20260711-131258/TASK.md -->
# do...end sequencing

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Candidates (user, 2026-07-09): A, pure sugar to bind-and-discard chains; B, statement lists run by a BLOCK frame.
- Decision (user, 2026-08-30): B. The opcode machinery executes `do ... end`; it is not desugared.
<!-- END tasks/20260711-131258/TASK.md -->

<a id="20260711-131259"></a>

<!-- BEGIN tasks/20260711-131259/TASK.md -->
# Proper tail calls

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Decision (user, 2026-08-30): proper tail calls are an obligation while writing the CESK rules.
- Frames arise only when a sub-result is needed; every other transition replaces control.
- Caveat (2026-07-09): storage still grows per iteration until reclamation exists.
<!-- END tasks/20260711-131259/TASK.md -->

<a id="20260718-175927"></a>

<!-- BEGIN tasks/20260718-175927/TASK.md -->
# Rule 3 dispatches on storage arity, wrong for APPLY

- STATUS: OPEN
- PRIORITY: 100
- TAGS: reducer, machine
- KIND: TASK
- PARENT: 20260529-103914

- [fact] `public/reducer/reducer.c` Rule 3 switches on storage arity. That is wrong for `APPLY` and other non-nyad cells.
- [fact] The regression dates from commit `e0b7b3f`.
- Rule 3 never evaluates (user, 2026-09-25): see 20260529-103914. For this code: Rule 3 must request the argument's triage view without evaluating it; an application is observed as a node of arity 2.
<!-- END tasks/20260718-175927/TASK.md -->

<a id="20260805-162236"></a>

<!-- BEGIN tasks/20260805-162236/TASK.md -->
# Hand-written spec in docs/new_spec

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: spec
- KIND: TASK

- (user, 2026-08-05) The spec is hand-written in `docs/new_spec`. The old `docs/spec` stays as history.
- ~~(user, 2026-08-30) Spec text is concise and normative. Reasoning goes into tasks.~~ moved to AGENTS.md (2026-09-26).
- ~~(user, 2026-08-30) The user writes the spec; the agent makes targeted edits only, in plain English.~~ moved to AGENTS.md (2026-09-26).
- ~~(2026-08-07) explicit delta arity `^0`, `^1[x]`, `^2[x, y]`~~ superseded by nyads, 2026-08-30.
- Spec references: `tatr ls :spec`.
- Session notes: archive/20260807-120000/2026-08-07.md, 2026-08-30.md.
- Dissolved (user, 2026-09-26): spec work is placed under the record of the concept it defines; the `spec` tag marks it (`tatr ls :spec`).
<!-- END tasks/20260805-162236/TASK.md -->

<a id="20260805-162237"></a>

<!-- BEGIN tasks/20260805-162237/TASK.md -->
# Explain each inspiration source in the intro

- STATUS: OPEN
- PRIORITY: 100
- TAGS: framing, spec
- KIND: TASK

- (user, spec) Explain each influence: tree and triage calculus, lambda calculus, C, ML, Scheme.
- Each needs a source link and the Vetochka level it shaped.
- Open (agent, 2026-09-26): placement. It defines no concept, and no intent fits yet, so it stays a provisional root task.
<!-- END tasks/20260805-162237/TASK.md -->

<a id="20260805-162238"></a>

<!-- BEGIN tasks/20260805-162238/TASK.md -->
# Complete the node type and pith list

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20260926-050129

- [fact] The spec lists only `nyad0..2` node types, each with its pith.
- (user, spec) A pith is the introspectable internals of a node, and it is dual: a pith built from scratch represents the same node.
- (user, 2026-08-30) Closure pith: declarators and body syntax, no env slot. Code closure pith: selected opcode and inputs as written.
- (user, 2026-08-30) LENS was renamed to pith.
- Open (agent, 2026-08-30): the pith of an un-executed `{@}` datum.
- Decision (user, 2026-09-24): every node is observable. A truly opaque node shows a minimal pith, for example a plain integer. Rule 3 never traps for lack of a pith.
- ~~(2026-07-11) a node whose kind has no pith traps at Rule 3 (`SHAPE-NO-LENS` in `docs/spec/03_v0.md`)~~ superseded by the ruling above. It only stood in for unfinished pith definitions.
- [fact] Since 2026-08-30 TERMs and machine frames are machine-internal, never data, so they never reach Rule 3.
- Open for the spec's Pith section (user writes): every node has a pith; a pith shows one layer and its children are piths; nyad-built terms are their own pith, primitive node types define theirs.
- Decision (user, 2026-09-24): storage and observation may differ; the pith is the bridge. This is the hermetic principle in action.
- Decision (user, 2026-09-24): piths are runtime semantics (Rule 3 and CESK), separate from node generation.
- Direction (user, 2026-09-25): piths are views into a node, a special reference with state. Rule 3 advances a pith, and it can be stored in a variable, keeping the view. Encoding (e.g. an i64) is secondary. (agent) This matches July LENS: inspection state, derived by Rule 3, unforgeable, non-callable.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
- Open (agent, 2026-09-26): the pith model Rule 3 works on: the node itself, or a view object Rule 3 advances. Needed by the machine core slice; decided in 20260926-091318.
- Research context (agent, 2026-10-19): see `../20260926-091318/2026-10-19-research.md` for identity and reconstruction hypotheses. No existing ruling is superseded.
- Requirement (user, 2026-10-19): observations are versioned parts of the language. Programs must be able to inspect their versions in runtime data, beyond a declaration such as `requires v1.2.3`.
- Example (user, 2026-10-19): `[tag, [version: 1.2, ...], ...]`. Metadata representation belongs to 20260903-085028. Exact version syntax and encoding remain open.
- Open (agent, 2026-10-19): what each version identifies, how versions coexist, and whether version data only describes an observation contract or also selects behavior.
<!-- END tasks/20260805-162238/TASK.md -->

<a id="20260805-162239"></a>

<!-- BEGIN tasks/20260805-162239/TASK.md -->
# Experiment: alternatives to lambda abstraction

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: binders, experiment
- KIND: TASK
- PARENT: 20241225-140425

- Decision (user, 2026-07-29): lambda plus triage stays the computation model. Abstraction remains a recognizable runtime tree form.
- Rejected: stack execution, rewriting, message passing or interaction nets as the universal model.
- Open then: runtime shape of abstractions and captures, what piths expose, partial application, mutation.
- This is the ancestor of the binder axiom (2026-08-30).
- Details: archive/20260717-120000/basis-experiment.md.
<!-- END tasks/20260805-162239/TASK.md -->

<a id="20260830-111134"></a>

<!-- BEGIN tasks/20260830-111134/TASK.md -->
# Spec the let binding

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, spec
- KIND: TASK
- PARENT: 20241225-140425

- Decision (user, 2026-08-30): one form, `let[clauses] do ... end`. The loose block ends it.
- Clause kinds are infix operators: `=` binds, `<~` binds recursively, more can be added. Clauses lower to `[{:infix}, op, id, expr]`.
- Scoping is sequential. A list-shaped clause is a knot, like OCaml `and`, or `let rec ... and` with `<~`.
- `_ = expr` runs an effect. Blocks inside clauses need parens.
- [fact] The spec still shows the old `$let: x is:` sketch.
<!-- END tasks/20260830-111134/TASK.md -->

<a id="20260830-165615"></a>

<!-- BEGIN tasks/20260830-165615/TASK.md -->
# Spec the binder discipline

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, spec
- KIND: TASK
- PARENT: 20241225-140425

- Settled direction (user, 2026-08-30), not yet in the spec by choice. Details: binders.md.
- TERM pairs a term with an env inside the machine only. The closure is the only env-carrying value.
- Values are embedded by building with application, behavior is kept with a thunk, and open syntax rebinds where it lands.
- Invariant: declared-no-value bindings exist only inside closures waiting for saturation.
<!-- END tasks/20260830-165615/TASK.md -->

<a id="20260830-165616"></a>

<!-- BEGIN tasks/20260830-165616/TASK.md -->
# Define $form

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, machine
- KIND: TASK
- PARENT: 20241225-140425

- Decision (user, 2026-08-30): the opcode that takes raw inputs is `$form`, in continuity with `{form}`. Callee-dependent raw arguments are accepted by design.
- Open (user): its exact shape is subject to change.
- ~~(2026-07-11) `$form` is not v0~~ superseded 2026-08-30.
<!-- END tasks/20260830-165616/TASK.md -->

<a id="20260831-155152"></a>

<!-- BEGIN tasks/20260831-155152/TASK.md -->
# Paren-statements candidate

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, rejected
- KIND: TASK
- PARENT: 20241220-144435

- Candidate: `(a; b)` as a block, dropping `do ... end`. It loses the loose trailing block `f do ... end`.
- ~~Decision (user, 2026-08-30): deferred until a bootstrap corpus in the current grammar decides.~~ superseded by the 2026-09-24 ruling.
- Rejected (user, 2026-09-24): no paren-statements. `do ... end` stays the block form. R1 gluing stays.
- Follow-up: the grouping question became task 20260924-092637.
- Details: archive/20260807-120000/labels-kv.md.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260831-155152/TASK.md -->

<a id="20260903-085028"></a>

<!-- BEGIN tasks/20260903-085028/TASK.md -->
# Metadata list in lowered nodes

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20241220-144435

- Decision (user, 2026-08-30): lowered nodes carry a metadata list after the type tag: `[{node}, [version, line, file, ...], ...payload]`.
- Spans and provenance live in the tree, not in side tables. Programs can observe line numbers through it.
- ~~(2026-07-09) spans in a host side table keyed by cell index~~ superseded 2026-07-10, then by this.
- Open: the exact list, and how piths treat the slot.
- Decision (user, 2026-09-24): `meta` sits right after the tag of every lowered node: `[{node}, meta, ...payload]`. It is a k-v list whose fields depend on the node. Now in the spec's rewrite rules.
- ~~Open: rules without a tag slot have no `meta` place yet, although every node carries metadata.~~ resolved (user, 2026-09-24): lists, nyads and applications carry no metadata. Byte strings need a slot.
- ~~Open: the lowered shape of a byte string with its `meta` slot.~~ superseded (user, 2026-09-24): byte strings carry no `meta`; applications do, `[{@}, meta, ...]`.
- Decision (user, 2026-09-24): `parse_term` may have a mode where every node carries metadata, for example to locate a list. Off by default, since meta on every node would recurse: meta needing its own meta.
- Open (agent): what that mode does with the meta lists themselves.
- (user, 2026-09-24) No recursion: meta lists are inspected, not given meta of their own, for example via a `{:meta}` slot. The slot may be a k-v entry, see task 20260924-111836.
- (user, 2026-09-24) Metadata is data about data: what you need to inspect a value, not to use it through functions. Some nodes should have it. Concern: if only some nodes have metadata, a node can't be inspected reliably without first knowing its kind.
- ~~Decision (user, 2026-09-24): assume every node carries some metadata, possibly empty.~~ superseded the same evening: lists, nyads and applications carry none (resolved open item above), later refined for applications.
- Open (user, 2026-09-24): inspection should show metadata, so every pith would include it by default. That suggests two layers, language and representation, where language-level metadata need not be stored in the node itself. Debatable: it may break the hermetic principle ("As above, so below").
- Decision (user, 2026-09-24): applications carry `meta`: `[{@}, meta, ...]`. (agent) Consistent with the head rule: `{@}` already heads the node.
- Decision (user, 2026-09-24): byte strings carry no `meta`. Their meaning comes from the surrounding node, e.g. `{x}` inside `[{:id}, meta, {x}]`.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
- Scope extension (user, 2026-10-19): the version field must support the observation-versioning requirement in 20260805-162238. This gives the previously listed version metadata a concrete use.
- Open (agent, 2026-10-19): choose the version value's encoding when specifying the field. The user's `1.2` example does not by itself introduce decimal literals or a compatibility policy.
<!-- END tasks/20260903-085028/TASK.md -->

<a id="20260903-085029"></a>

<!-- BEGIN tasks/20260903-085029/TASK.md -->
# Specify exec, the host entry

- STATUS: OPEN
- PRIORITY: 100
- TAGS: host, machine, spec
- KIND: TASK
- PARENT: 20260613-151556

- (user, 2026-08-30) `exec` is a host-side, top-level, apply-once entry. It forces a value term containing `{@}`.
- Direction (user): `exec` is likely not user-accessible; applying one value to another already interprets code in the language.
- Open: its exact semantics in the spec.
- Direction (user, 2026-09-24): `exec` follows a meta-ignoring principle. Turning `[{@}, meta, f, x]` into a machine application drops `meta`, as Jay's tags never change what a function does (tree book 5.4).
- (agent) Refinement, not ruled: the rule covers all execution, `exec` and every opcode that reads raw syntax, since those nodes carry `meta` too. Wording: `meta` never changes what execution computes; reading it for diagnostics is allowed.
- Evidence on `{@}` execution, inspection and forcing: apply-evidence.md (2026-09-25).
- Decision (user, 2026-09-26): `exec` drives the computation one step at a time. Given the evaluation rules, one applicable rule is chosen and executed. No deep evaluation: the CESK rules define evaluation completely, with corner-case prose where needed, as with the grammar.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
- Open (agent, 2026-09-26): who repeats the one-step `exec` until a value: the host, or a driver the spec defines. Decided in 20260926-091318.
<!-- END tasks/20260903-085029/TASK.md -->

<a id="20260911-112058"></a>

<!-- BEGIN tasks/20260911-112058/TASK.md -->
# Complete-picture run

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: experiment, spec
- KIND: TASK
- PARENT: 20260926-050132

- Experiment (user ruling, 2026-09-24): an agent allowed to imagine produced a whole-pipeline picture. Its content is hypothesis, not ruling.
- Its spec edits were reverted. The diff is in archive/20260903-120000/experiment-2026-09-03-spec-edits.md.
- Its open questions O1 to O8 are not tasks until the user adopts them.
- Attachments: archive/20260903-120000/picture.md, archive/20260903-120000/2026-09-03.md, archive/20260903-120000/experiment-2026-09-03-spec-edits.md.
<!-- END tasks/20260911-112058/TASK.md -->

<a id="20260924-074843"></a>

<!-- BEGIN tasks/20260924-074843/TASK.md -->
# Move task tracking to tatr

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process
- KIND: TASK
- PARENT: 20260926-050132

- Decision (user): task tracking uses tatr. `docs/` holds user-facing docs; `tasks/` holds the record of design work.
- Decision (user): tasks are the source of truth for design labour, as important as code. They state current knowledge, not migrated quotes.
- Decision (user): epics first, tagged `epic`. Sub-items that are not tasks on their own go in the epic's `items.md`.
- Decision (user): task ID is the time the note was first written. Collisions take the next free second.
- Decision (user): docs and code reference tasks by ID instead of TODOs.
- Decision (user): `dialogue/`, `project/` and `.memsearch/memory/` are abandoned. Their files live in task folders.
- Decision (user): each project has its own memsearch collection (`vetochka`, in `.memsearch.toml`), indexing `tasks/` and `docs/`. The shared default collection is to be purged.
- Decision (user): the repo-local memory skill is removed. The tatr skill carries the task-as-memory rules.
- Decision (user): built spec PDFs (`docs/*/output.pdf`) are gitignored.
- Open: purge of the shared `memsearch_chunks` collection was blocked by permissions; the user runs it.
- Details: archive/20260924-120000/2026-09-24.md.
- Decision (user, 2026-09-26): a third artifact kind, `archive/`, for garbage-collected process notes; see AGENTS.md.
<!-- END tasks/20260924-074843/TASK.md -->

<a id="20260924-083540"></a>

<!-- BEGIN tasks/20260924-083540/TASK.md -->
# Project license

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process
- KIND: TASK
- PARENT: 20260926-050132

- Decision (user, 2026-09-24): code is GPL-3.0-or-later (`LICENSE`).
- Decision (user, 2026-09-24): docs and design records, including AI output, are CC BY 4.0 (`LICENSE-DOCS`); it is public knowledge.
- Decision (user, 2026-09-24): generated code is not subject to the project license.
- [fact] Vendored code keeps its own licenses, all compatible with GPL-3.0: `stb_ds.h` MIT or public domain, `da.h` public domain, `arena.h` Apache-2.0 (Carter Dugan, 2024).
- [fact] tatr (GPL-2.0, Alexey Kutepov) is only a tool. `tasks/README.md` is our own text.
- [fact] `docs/smart/*.pdf` are third-party works outside the project license. SML Definition and Lafont's paper are likely not redistributable; the tree book is unknown.
- Done: `LICENSE`, `LICENSE-DOCS`, README license section, copyright line `Copyright (C) 2024-2026 uberdever` (user, 2026-09-24).
- Deferred: resolve `docs/smart/*.pdf` (remove or link). Decision (user, 2026-09-24): keep for now, likely remove later.
- Deferred: permissive carve-out for examples users copy into their programs.
- Deferred: a runtime-library exception if the compiler ever copies its own code into output.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-083540/TASK.md -->

<a id="20260924-092637"></a>

<!-- BEGIN tasks/20260924-092637/TASK.md -->
# Lowering of mixed infix chains

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20241220-144435

- [fact] Rewrite rule 12 in `docs/new_spec/02_syntax.md` shows a single operator: `x op y op z -> [{:infix}, {op}, x, y, z]`. It doesn't say how a chain with different operators lowers.
- Decision (user, 2026-09-24): operators in a chain may differ. Without parens the whole chain is flat, in one list.
- [fact] Parens already keep grouping apart: `(a + b) * c` has the lowered `a + b` as one operand, while `a + b * c` is one flat chain.
- Decision (user, 2026-09-24, spec rule 12): `x op1 y op2 z -> [{:infix}, [{op1}, {op2}], x, y, z]`. Operators go in a list; operands stay flat. vf stages analyze them.
- Background: paren-statements task 20260831-155152, closed the same day.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-092637/TASK.md -->

<a id="20260924-100255"></a>

<!-- BEGIN tasks/20260924-100255/TASK.md -->
# Reword cells: runtime nodes with non-trivial piths

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding, spec
- KIND: TASK
- PARENT: 20260925-134730

- [fact] `docs/new_spec/02_syntax.md`, Cells: "Every node is typed ... and has its arity: `0, 1 or 2`. Note that semantically a node could have any amount of children if proper list encoding is used." Written by the user, 2026-08-05.
- Decision (user, 2026-09-24): the idea stays; the wording changes. There are special runtime nodes throughout, a "runtime category" with non-trivial piths, and they may have more than two children.
- Reminder for the user: rewrite that paragraph in the spec.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-100255/TASK.md -->

<a id="20260924-105828"></a>

<!-- BEGIN tasks/20260924-105828/TASK.md -->
# First-class heads for special kinds

- STATUS: OPEN
- PRIORITY: 100
- TAGS: optimization
- KIND: TASK
- PARENT: 20260925-134730

- (user, 2026-09-24) Which kinds get their own head, like `{:closure}`, and which go under a category with the kind in `meta`. Like core opcode shorthand versus `$ns: {namespace} op: {opcode}`.
- (user+agent, 2026-09-24) Rule of thumb: a kind is special enough when the machine or Rule 3 dispatch needs it on the first node.
- (user, 2026-09-24) Related: storage may compress trees, for example not storing string tags in list entries. That may only affect what Rule 3 sees, not meaning.
- (user, 2026-09-26) Designators are short special strings at the head of a list; which of them the evaluator recognizes fast is this optimization question.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-105828/TASK.md -->

<a id="20260924-111836"></a>

<!-- BEGIN tasks/20260924-111836/TASK.md -->
# K-V lists as a language concept

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode
- KIND: TASK
- PARENT: 20241220-144435

- Direction (user, 2026-09-24): refine k-v lists into a language concept. They give optionality and free order. Metadata could be an optional k-v entry, present or absent, leaving the positional node well-formed.
- (user, 2026-09-24) Restriction: k-v entries go after positionals, enforced by the parser.
- [fact] Today labeled expressions are ordinary entries (C6, 2026-08-30): `[x: 2, y: 4]` is already a k-v list, and `label: expr` lowers to `[{:label}, meta, {label}, expr]`. Details: archive/20260807-120000/labels-kv.md.
- (agent) Challenge: blocks and lists may already contain labeled entries, so a trailing `meta:` entry can collide with payload. It needs a reserved key or tag.
- (agent) Conflict: the spec now puts `meta` positionally after the tag. A trailing optional k-v entry would replace that.
- (agent) Prior art, from general knowledge: Python keyword arguments (positionals first, parser-enforced), Elixir keyword lists (plain lists of pairs, sugar in last position), Lua tables (positional and keyed parts in one table), Common Lisp `&key` plists.
- Decision (user, 2026-09-24): `meta` is a reserved key, special like Lua metatables or Python's special fields. No other reserved keys.
- (user, 2026-09-24) Leaning: k-v `meta` over a positional slot that is sometimes empty. Not settled.
- (user, 2026-09-24) Idea: a list with zero or more k-v entries is a proper list; a pure k-v list has k-v entries only. Both fit the subtyping relation.
- (agent) Cost: a trailing `meta` entry is found by walking the whole payload; a block with a thousand statements walks a thousand cells. An optional `meta:` entry right after the tag keeps the lookup to one step, as an exception to "k-v after positionals".
- (agent) The subtypes are predicates on structure, like Scheme's `list?`: proper list, list with k-v entries, pure k-v list, tagged node. They are checked by walking, not declared.
- Decision (user, 2026-09-24): Vetochka has `meta`. The name is debatable.
- (user, 2026-09-24) A node's top level should be chunked: kind, meta, contents, data. It should never have a thousand top-level entries. A second-position `meta:` present on some nodes and absent on others is acceptable but less unified.
- (agent) [fact] Today's rewrite rules don't chunk: `do a; b end -> [{:block}, meta, a, b]` and `x op1 y op2 z -> [{:infix}, meta, [{op1}, {op2}], x, y, z]` put every statement or operand at the top level.
- Proposal (user, 2026-09-24, unreliable narrator): every node is `[{:stuff}, [meta-or-empty], ...]`. The meta list is always present, possibly empty; its fields are k-v entries, so each field is optional.
- Open: where the payload goes. Chunked reading (agent): `[{:stuff}, [meta...], payload]`, one payload entry.
- Leaning (user, 2026-09-24, unreliable narrator): `[{:stuff}, [meta...], payload]` rather than an embedded list, to avoid an extra node for the embedding on every node.

- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-111836/TASK.md -->

<a id="20260924-120458"></a>

<!-- BEGIN tasks/20260924-120458/TASK.md -->
# Bring the parser up to date with the spec

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, reducer
- KIND: TASK
- PARENT: 20241220-144435

- (user, 2026-09-24) Syntax here means parsing: generating nodes. Inspection (Rule 3, CESK, piths) is a separate step and may stay open. Planned as a separate session.
- [fact] Gaps in `reducer/` against `docs/new_spec/02_syntax.md`:
  - `^` is still lexed as a delta node (`source_tokenize.c`), from before the nyad rename; nyads are `~[...]`.
  - `~` is still a unary operator; it is now the nyad sigil.
  - Operators come from a fixed unary list, not the ADR 1 adjacency classification.
  - The encoder emits a `:group` node (`bytecode_source.c`); rule 4 erases parens.
- Pending decisions that change generated nodes:
  - The `meta` slot: position, always-present list, chunked payload. Task 20260924-111836.
  - Applications as `[{@}, meta, ...]`. Task 20260209-162003.
  - A rewrite rule for literals.
- Decision (user, 2026-09-24): the encoder emits applications as `{@}` data, `[{@}, meta, f, x]`, not APPLY cells. Per 2026-08-30, APPLY exists only in machine control; parser output is data.
- Decision (user, 2026-09-24): `meta` is inert data, for inspection only.
- Decision (user, 2026-09-24): `meta` is an empty list by default; source position only when the caller of the parse routine asks for it.
- Decision (user, 2026-09-24): follow the spec's ASI rule: no exception after an annotation's `]`. Write annotations on the same line as their expression.
- Decision (user, 2026-09-24): drop the `{fn}` / OP_FN0 recognition from the encoder; `$fn:` superseded it.
- Done (agent, 2026-09-24): lexer, parser, formatter and encoder follow `docs/new_spec/02_syntax.md`. Token classes and lexer state live in `reducer/source_impl.h`. The encoder takes `bytecode_source_options_t` by value (user); `source_meta` fills `meta` with `[line: n, col: n]`.
- [fact] `#;` is trivia for ASI: a `;` caused only by the discarded expression is dropped.
- ~~[fact] `^` and `Δ` are lexing errors~~ superseded (user, 2026-09-24): `^` is an operator character (spec updated); ASCII-only identifiers are an implementation drawback, task 20260924-123410.
- [fact] Tests: 25 pass, also under `-Dsanitize=true`. Rejection tests cover ADR 1, R1, nyad arity, `$`, `@`, and an annotation on its own line.
- Known nit: the canonical formatter writes two spaces before a `...` line break (older behaviour).

- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-120458/TASK.md -->

<a id="20260924-123410"></a>

<!-- BEGIN tasks/20260924-123410/TASK.md -->
# Unicode identifiers

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, reducer
- KIND: TASK
- PARENT: 20241220-144435

- Decision (user, 2026-09-24): ASCII-only identifiers are a drawback of the current implementation, not a restriction of the language.
- [fact] The spec's grammar still reads `identifier_start ::= [a-zA-Z_]`, and the lexer follows it: `Δ` is rejected.
- Open: which non-ASCII characters identifiers admit (the spec rule), then the lexer.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
<!-- END tasks/20260924-123410/TASK.md -->

<a id="20260925-134730"></a>

<!-- BEGIN tasks/20260925-134730/TASK.md -->
# Cells revision

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding
- KIND: INITIATIVE
- PARENT: 20260926-050129

- Goal (user, 2026-09-25): introduce an abstraction between cells (storage) and the in-language representation, mainly for piths.
- Storage and observation may differ; piths bridge them (user, 2026-09-24): see 20260805-162238.
- [fact] Current encoding: cells-encoding.md (from `public/cells/impl.h`, `api.h`, `node.c`).
- [fact] Cells still carry `OP_FN0..2` and `APPLY`, while the encoder no longer emits either: applications are `{@}` data since 2026-09-24.
- (user, 2026-09-25) Wire bytes from `0x80` leave room for about 42 families in three arities, more for single-arity tags. Today's tags separate storage layouts: nyad, i64, bytes, some opcodes. Direction: tie tags to the semantic objects behind the language instead; `APPLY` and some opcodes already are.
- Direction (user, 2026-09-25): as above, so below — executing requires an exactly shaped node. For example `{@} lhs: f rhs: x` rather than any list headed by `{@}`, so it pairs with the cell form `APPLY lhs: ref-to-f rhs: ref-or-inline-x`.
- (agent) Tension: 2026-08-30 put APPLY in machine control only and application in data as the `{@}` marker; the encoder emits `[{@}, meta, f, x]` lists (2026-09-24). An exact-shaped application node stored as `APPLY` would make data and control share one cell form, with `exec` as the only door. Needs a ruling.
- Next (user, 2026-09-25): categorize current runtime object kinds, grammar first, then execution rules. First pass: object-kinds.md.
- Decision (user, 2026-09-25): runtime objects are first-class, above all storable. Machine state (pending applications, TERM) is not a runtime object.
- ~~(agent) Open tension for the CESK rules: 2026-08-30 rules both "Rule 3 does not force; identifier dereference is the only indirection" and "a pending machine application in the operand is evaluated to expose its root shape". The exception only matters if operands can be pending when Rule 3 fires: yes under July's callee-first `RUN-APPLY`, no under branch-first evaluation as pinned the same day. Task 20260711-131257.~~ resolved by the 2026-09-25 ruling below.
- Rule 3 never evaluates (user, 2026-09-25): see 20260529-103914.
- ~~Open: what Rule 3 sees of an application depends on this task: a `[{@}, meta, f, x]` list is a list cell; an exact `APPLY` node is a fork of `f` and `x`.~~ corrected (user, 2026-09-25): an application, like any runtime node, may have its own pith, so storage and observation are separate choices.
- Open: the pith of an application (task 20260805-162238) and, separately, its storage (this task).
- Opcodes are designator-headed lists (user, 2026-09-26): see 20250123-091611.
- (agent) This answers the storage question for opcodes and `{@}`: lists with a designator head, not per-state cell tags. The exact-shape direction of 2026-09-25 now reads as: the designator list is the exact shape.
- Session notes: 2026-09-25.md, 2026-09-26.md.
<!-- END tasks/20260925-134730/TASK.md -->

<a id="20260926-043840"></a>

<!-- BEGIN tasks/20260926-043840/TASK.md -->
# Restructure tasks into an intent tree

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process
- KIND: TASK
- PARENT: 20260926-050132

- Work (user, 2026-09-26): restructure `tasks/` into Intent → Initiative → Task per the tatr skill: `PARENT` as the only structural link, each claim in one record, categories as tags. Option B, one pass, intents designed first.
- Done when: every record has a `KIND` and a justified `PARENT` (or is a provisional root task), no claim is copied across records, and no child lists or "Related" links remain.
- ~~Open (agent): its own parent, once the intents exist.~~ resolved: parent is the bookkeeping intent.
- Dialogue ledger: intents-dialogue.md.
- Mapping proposal: mapping.md (not applied).
- Done (user, 2026-09-26): committed as `aff6565`. Every record has a KIND and a justified PARENT (or is a provisional root task); no claim is copied across records; no child lists or Related links remain. A post-apply check found and restored 4 rulings lost from their owners, plus 2 meta decisions.
<!-- END tasks/20260926-043840/TASK.md -->

<a id="20260926-050128"></a>

<!-- BEGIN tasks/20260926-050128/TASK.md -->
# One syntax, expressive for people and trivial for programs

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: INTENT

- Desired condition (user, 2026-09-26): one syntax that is expressive for people and trivial for programs to analyze and construct. Intro statement S1.
- Why (user): intro motives M3, M4, `docs/new_spec/01_intro.md`.
<!-- END tasks/20260926-050128/TASK.md -->

<a id="20260926-050129"></a>

<!-- BEGIN tasks/20260926-050129/TASK.md -->
# Programs observe and build any program as data

- STATUS: OPEN
- PRIORITY: 100
- TAGS:
- KIND: INTENT

- Desired condition (user, 2026-09-26): any program can observe and build any program as ordinary data, without quotation. Intro statements S2 (reflection: one term representation, one introspection rule) and S3 (intensionality).
- Why (user): intro motives M4, `docs/new_spec/01_intro.md`.
<!-- END tasks/20260926-050129/TASK.md -->

<a id="20260926-050130"></a>

<!-- BEGIN tasks/20260926-050130/TASK.md -->
# New languages are defined by programs over a shared core

- STATUS: OPEN
- PRIORITY: 100
- TAGS:
- KIND: INTENT

- Desired condition (user, 2026-09-26): new languages are defined by programs over a shared executable core, not by changing the core. Intro statements S4 and S5; S5's "any compiler to any language" is the aspiration, not a checkable condition.
- Why (user): intro motives M3, M4, `docs/new_spec/01_intro.md`.
<!-- END tasks/20260926-050130/TASK.md -->

<a id="20260926-050132"></a>

<!-- BEGIN tasks/20260926-050132/TASK.md -->
# Bookkeeping keeps design knowledge current and findable

- STATUS: OPEN
- PRIORITY: 100
- TAGS: process
- KIND: INTENT

- Desired condition (user, 2026-09-26): project bookkeeping keeps design knowledge current and findable with little effort.
- Why (user, 2026-09-26): the user spends a lot of time on bookkeeping.
<!-- END tasks/20260926-050132/TASK.md -->

<a id="20260926-053139"></a>

<!-- BEGIN tasks/20260926-053139/TASK.md -->
# Lowered form of literals

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20241220-144435

- Decision (user, 2026-09-26): to preserve "as above, so below", literals come in two tagged forms: a pure tree built from nyads, and a tree with an opaque payload. The two probably get different tags. Motivation: a string can be built out of nyads, and a string can be disassembled into nyads.
- Done when: the spec has a rewrite rule for integer and byte-string literals in both forms, and the encoder emits them.
- [fact] (agent) Today the encoder emits bare native leaves for literals (`bytecode_source.c`, `encode_token`).
- (agent) This revisits rulings from 2026-09-24, marked for sober review: general values are leaves (20260209-162003); byte strings carry no `meta` (20260903-085028). Pure-tree literals follow Jay's encoding (tree book 3.7, 4.7).
- Open (agent): the tag names; whether both forms carry `meta`; whether names and designators inside tagged nodes (`{x}` in `[{:id}, meta, {x}]`) stay bare leaves.
<!-- END tasks/20260926-053139/TASK.md -->

<a id="20260926-091318"></a>

<!-- BEGIN tasks/20260926-091318/TASK.md -->
# Settle value representation

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, encoding
- KIND: TASK
- PARENT: 20260925-134730

- Step 0 of the v0 plan (user+agent, 2026-09-26): decisions only, before any machine C code. Plan: 20260529-103914/2026-09-26.md.
- Work (user, 2026-09-26): sober review of the rulings marked "Review sober" (2026-09-24), in 20260209-162003, 20260903-085028, 20260805-162238, 20260903-085029, 20260606-141938.
- (agent) [fact] Three rulings pull apart: general values such as i64 and strings are leaves (20260209-162003, 2026-09-24); opaque nodes are distinguished by the tag first in their list (same task, same day); literals come in two tagged forms (20260926-053139, 2026-09-26).
- Decide: one value representation for integers, byte strings and designators; the fate of the `APPLY` and `OP_FN0..2` cell tags; the pith model Rule 3 works on (open in 20260805-162238); who repeats the one-step `exec` (open in 20260903-085029).
- Done when: each decision above is recorded as a user ruling in its owning task.
- Discussion (user+agent, 2026-10-19): syntax, reduction, and runtime model in `2026-10-19.md`. Representation choices remain open.
- Research (agent, 2026-10-19): `2026-10-19-research.md` compares the hypotheses with contextual equivalence, environmental bisimulation, tree calculus, reflection, and algebraic effects. No new design ruling.
<!-- END tasks/20260926-091318/TASK.md -->

<a id="20260926-091320"></a>

<!-- BEGIN tasks/20260926-091320/TASK.md -->
# Machine core slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, reducer, spec
- KIND: TASK
- PARENT: 20260529-103914

- Step 1 of the v0 plan (user+agent, 2026-09-26). Needs 20260926-091318 and the build migration 20260606-141938.
- Work: CESK state in C; one-step `exec`; triage rules 0a..3c; identifier dereference; hard traps. Includes the Rule 3 fix, 20260718-175927.
- Spec side: the matching CESK rules in `03_v0.md` (20260711-131257), written with the code.
- Done when: the rules are in the spec, and tests run nyad-only and identifier programs through `exec` to a value, including Rule 3 on an application.
<!-- END tasks/20260926-091320/TASK.md -->

<a id="20260926-091321"></a>

<!-- BEGIN tasks/20260926-091321/TASK.md -->
# Opcode protocol and binders slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, opcode, binders, spec
- KIND: TASK
- PARENT: 20260529-103914

- Step 2 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091320.
- Work: a generic dispatcher for designator-headed lists with the Continue/Dispatch/hard-trap states (20250123-091611); then `$fn` closures with proper tail calls, `let[...]` (20260830-111134) and `$form` (20260830-165616), under the binder discipline (20260830-165615).
- Done when: the spec has their CESK rules, and tests cover currying, saturation, shadowing, tail calls in constant stack, and traps.
<!-- END tasks/20260926-091321/TASK.md -->

<a id="20260926-091323"></a>

<!-- BEGIN tasks/20260926-091323/TASK.md -->
# Native operations slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode, syntax
- KIND: TASK
- PARENT: 20250123-091611

- Step 3 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091321.
- Work: integer and byte-string literals in both forms (20260926-053139); i64 arithmetic and comparison; byte-string operations; native tag equality (ruled inevitable, 20260209-162003); native calls as trampolines with no machine re-entry from C (20260711-131257).
- Open (agent): the v0 set of native operations beyond these.
- Done when: the opcodes are specified, and tests run arithmetic, comparison and string programs through `exec`.
<!-- END tasks/20260926-091323/TASK.md -->

<a id="20260926-091324"></a>

<!-- BEGIN tasks/20260926-091324/TASK.md -->
# Piths for every runtime kind

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding, machine
- KIND: TASK
- PARENT: 20260925-134730

- Step 4 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091323; the pith model is decided in 20260926-091318.
- Work: a pith for each kind in `20260925-134730/object-kinds.md`: nyad, list, integer, byte string, application, identifier, closure, opcode state, native handle, and the pith itself. Syntax-only kinds are plain lists. Definitions live in 20260805-162238.
- Scope (user, 2026-09-26): the object-kinds inventory for now; further kinds such as boxes (20260604-111747) later.
- Done when: every kind has a specified pith, and tests show duality: a pith built from scratch represents the same node.
<!-- END tasks/20260926-091324/TASK.md -->

<a id="20260926-091325"></a>

<!-- BEGIN tasks/20260926-091325/TASK.md -->
# Host rim slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: host
- KIND: TASK
- PARENT: 20260613-151556

- Step 5 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091324.
- Work: the host entry `exec` (20260903-085029); `load_file`, `parse_term` (reusing `public/source/`), print and trap, modeled on WASI; a bootstrap v0 file that loads and runs a program.
- Done when: an executable runs a hello-world v0 program from a file end to end.
<!-- END tasks/20260926-091325/TASK.md -->

<a id="20260929-125822"></a>

<!-- BEGIN tasks/20260929-125822/TASK.md -->
# Canonical form is not a fixed point

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: TASK
- PARENT: 20241220-144435

- [fact] (agent, 2026-09-29) Formatting canonical text again changes it: `format("f(x,   y)")` is `"f(x, y)\n"`, and formatting that gives `"f(x, y);\n"`: the trailing newline comes back as `;`. Found while writing the first Lua test (`source_tree_format_canonical` through the Lua module's `format`).
- [fact] What is claimed today: the canonical form parses again (the former Zig test `source formatters smoke`, now in `tests/source_test.lua`). Nothing claims that formatting is idempotent.
- Open (user): should canonical form be a fixed point, `format(format(x)) == format(x)`? If yes: a test for it in `tests/source_test.lua`, and a fix in the formatter or the newline-based semicolon insertion.
<!-- END tasks/20260929-125822/TASK.md -->
