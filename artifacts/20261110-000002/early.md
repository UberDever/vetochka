### 25.12.24
- (#representation #decision) Store trees in contiguous memory for cache locality. Start with one array of 64-bit nodes because this implementation is simple.
- (#representation #evidence) Evaluation adds nodes, so implicit positional links would require copying. Explicit links also permit variable-size encoding, although the initial choice remains fixed-size.
- (#representation #proposal) Encode tree/application nodes with two 31-bit relative offsets and reserve an offset for absence. Reserve other node forms for interpreter intrinsics.
- (#scope #proposal) Use named `scope` expressions as modules and unnamed scopes as let-bindings. Evaluate bindings on demand, not as statements. Scopes disappear before tree-calculus representation.
- (#scope #question) The final scope expression supplies its value. The result of an empty scope remains unclear. Recursive bindings are not planned because combinators can provide recursion.
- (#module #decision) Use `root.tree` only as a discovery anchor. Reject executing setup code from it because build-time execution and module location are separate responsibilities.
- (#module #proposal) Discover modules recursively from the anchor. Interpreter arguments supply explicit `(prefix, root.tree)` mappings and an entry file. Single-file execution without mappings cannot use modules.
- (#module #proposal) Modules are named scopes with locally unique names under programmer control. Nesting imports outer bindings without changing module names. `use` introduces declarations into an expression, and aliases resolve clashes.
- (#scope #decision) Reuse scopes for let-bindings. Module declaration sugar may behave as a statement, despite reducing expression consistency, because it improves usability.

### 23.01.25
- (#host #question) A pure computation can expose only its final result, initially imagined as bytes. Interactive effects instead require interpreter intrinsics that can affect evaluation state.
- (#tagging #evidence) Function tags can preserve callable behavior, but tagging a value changes its structure. All consumers must then agree on boxing and unboxing.
- (#tagging #decision) Prefer native tagging over calculus-only encodings for speed and convenience. Accept stronger coupling to the interpreter and reduced portability or extensibility.
- (#tagging #proposal) Initially provide native natural numbers and operations while preserving calculus-like semantics where possible. Use intrinsic numbers as tags. Other native facilities can wait.

### 31.01.26
- (#v0 #proposal) Vetochka 0 is a minimal triage-calculus VM with enough bytecode to implement a higher functional language efficiently. Its byte-array `cells` stores binary trees with optional payloads.
- (#representation #proposal) Represent references, delta leaf/stem/fork nodes, and fixed or variable native payloads. Relative references address cells, and variable payloads carry a ULEB128 length.
- (#evaluation #proposal) The reducer writes reduction results rather than rewriting terms in place. It tracks pending applications, a value result index, and intermediate results.
- (#vm #proposal) Add a separate bytecode layer with a data pointer, environment stack, construction/application instructions, forcing, binding/loading, and scope markers. This places additional behavior outside the reducer.

### 01.02.26
- (#intensionality #evidence) A separate reducer cannot inspect effects outside its own state. Needed sequencing, bindings, mutation, and abstraction must therefore become visible within calculus-level data.
- (#opcode #proposal) Represent common operations as specially encoded tree nodes intercepted by native routines. This makes operations inspectable or generatable while avoiding expensive pure encodings.
- (#evaluation #proposal) `seq` evaluates and discards its first child, then returns its second child's value. A `set` operation and reference boxes could implement quoting, one-time binding, and repeated evaluation.
- (#function #proposal) A lambda header contains captured values, mutable locations, and parameters. Copy the header on invocation, not the body, so parameter indirection supports body reuse.
- (#representation #proposal) Treat tree bytecode as a byte stream with inline payloads. Variable-width references minimize overhead. An eight-byte reference slot can serve as a mutable box whose value changes size.

### 04.02.26
- (#representation #decision) Unify every node under leaf/stem/fork saturation rules 0a and 0b. Separate delta, native value, and opcode kinds. Behavior differs when a fork receives another argument.
- (#host #proposal) Apply fixed-payload values by interpreting their payload as a native function address. Applying variable byte payloads is an error. Opcodes execute VM-specific behavior.
- (#reflection #proposal) Provide `get_type` and `get_payload`, plus construction operations `set_type` and `set_payload`. Shape remains inspectable through triage rule 3. Tagged type representation remains unresolved.
- (#host #decision) Treat programs as trusted and native addresses as forgeable. Embeddings own safety. Changing a payload's type without validation can crash or invoke arbitrary native code. Enforce payload-kind matching in `set_payload`.

### 09.02.26
- (#representation #decision) Retain only two-byte and eight-byte relative references. Use byte tags for delta triples, fixed eight-byte values, variable byte values, and opcode triples. Opcode arguments are children.
- [ ] (#memory #proposal) Use a free list in `cells` for reclamation.

### 10.02.26
- (#host #decision) Native calls require a dedicated `call` opcode. Ordinary values are no longer callable, although saturation still builds their stem/fork forms.

### 12.02.26
- (#syntax #proposal) Explore Wisp for a textual v0, then called v1. Parentheses denote right-associative application in this draft. Lists require explicit delta construction.
- (#literal #decision) Give strings one lexical responsibility: delimit human-readable bytes. Use matching repeated braces, greedy delimiters, and no escapes. Move trimming, encoding, formatting, and termination to compile-time functions.
- (#literal #question) Compile-time execution is therefore needed. Numeric literal extensibility remains unresolved beyond an initial interest in 64-bit integers.

### 13.02.26
- (#syntax #proposal) Combine Wisp with SRFI-105 curly infix, but omit mixed-infix `$nfx` forms because no present use requires them. Sources: https://srfi.schemers.org/srfi-119/srfi-119.html and https://srfi.schemers.org/srfi-105/srfi-105.html.

### 15.02.26
- (#syntax #decision) Unite textual bytecode and source syntax under Vetochka. Ordinary parenthesized applications are now left-associative. Retain Wisp but remove curly infix from the built-in syntax.
- (#literal #proposal) Built-in `<prefix>{contents}` forms encode native values. Other prefixes lower to a pair of prefix and content strings. All prefixes share the string delimiter rules.
- (#syntax #proposal) Reference declarations name code locations and optional chains of reference boxes. Uses can share code or support mutation. Dot-prefixed names denote opcodes. Block and line comments use `;{}` and `;;`.
- (#syntax #decision) Keep one built-in syntax for extensions. Define a canonical subset with explicit applications, desugared literals, and no comments for dumping and persistence.
- [ ] (#parser #proposal) Implement the textual bytecode parser and bytecode dumping.

### 16.02.26
- (#syntax #decision) Keep infix interpretation outside the base reader because reflection can implement it. Add list sugar because explicit delta lists are cumbersome.
- (#literal #proposal) Consider removing literal prefixes: `{}` would mean UTF-8 bytes and `[]` a proper nil-terminated list. Numeric parsing would move to designated operations evaluated during a build step.
- (#evaluation #experiment) Applying adjacent nodes instead of explicit reducer markers fails to express scattered results of multistep triage rules. Lambda scheduling belongs above the reducer, so it does not justify this change.

### 17.02.26
- (#evaluation #decision) First implement and test the calculus reducer. Introduce each required encoding afterward and add opcodes only with understood semantics, rather than using them to bypass calculus questions.
- (#evaluation #proposal) Keep the syntax and conceptual mutation model, but reconsider sequencing and lambdas through calculus encodings. Implementation remains pending while other work interrupts the project.

### 22.02.26
- (#opcode #proposal) Keep triage reduction intact and encode opcodes as tree values, such as `^ [magic opcode payload...]`. Application supplies an argument list and makes a fork eligible for VM dispatch.
- (#evaluation #proposal) Alternate reducer normalization with VM big-step execution until `halt`. Encode sequencing with continuations and combine abstraction through combinators with specialized operations.
- (#calculus #evidence) The retained basis consists of delta saturation, constant selection, substitution, and shape triage over leaf/stem/fork arguments. These rules preserve structural inspection of code-like data.
- (#representation #proposal) Treat explicit `$` as explanatory notation. The record still considers positional application sufficient, rather than storing application nodes as a separate kind.

### 18.03.26
- (#opcode #proposal) LLM tagging research proposes a fixed-position magic native integer as opcode identity. Unsaturated stems remain reduced data. Saturated forks dispatch after validation of payload and arguments.
- (#opcode #proposal) The proposed convention carries identity, payload, and argument lists inside ordinary trees, permits constant-time host recognition, and retains calculus shape inspection. This is research, not an approved ABI. Source: artifacts/20261020-000000/20260201-183604--project_history.md, 18.03.2026.

### 24.03.26
- (#abi #proposal) Use one tagged representation across parser, VM, and programs: `^ [tag payload...]` for applicable stems and the same stem applied to `^` for tagged data forks.
- (#abi #proposal) Runtime-defined tags distinguish syntax categories sharing byte representation, such as identifiers and strings, and identify opcodes for VM effects. Programs can construct and inspect these forms. Versioning is possible but not a priority.

### 02.04.26
- [ ] (#grammar #proposal) Write a proper grammar with EBNF. Reconsider whether strict Lisp syntax actually fits the intended language.

### 06.04.26
- (#vm #decision) Prioritize an efficient VM with clear semantics while retaining triage rules and structural reflection. Do not require every opcode to reproduce the same computation as a pure-calculus encoding.
- (#vm #proposal) Treat the VM as a separate big-step machine with tree-shaped opcodes. Continuations can suspend child evaluation, and environments or frames can support lambdas and bound-variable access.

### 28.05.26
- (#purpose #decision) Reframe Vetochka as a reflective, staged frontend for C. Metaprograms inspect and generate declarations, types, and implementation fragments that lower to ordinary C and its ABI.
- (#purpose #decision) Preserve C's control, explicitness, simplicity, and openness. Avoid a replacement runtime or standard library. Added modules, macros, types, and DSLs must serve C generation.
- (#syntax #proposal) Replace the Wisp direction with statement-oriented, Elixir-inspired shapes. The parser recognizes calls, lists, strings, selectors, labels, blocks, and operator shapes rather than keyword semantics.
- (#syntax #proposal) Parentheses group expressions or form immediate calls. Brackets and braces distinguish adjacent indexing from separated payload application. Use commas for arguments/lists and semicolons for canonical statement separation.
- (#syntax #proposal) Recognize prefix/infix shape without precedence or mixed infix. Support expression annotations. Extensible interpretation, rather than additional parser keywords, should carry meaning.

### 30.05.26
- (#function #proposal) Support currying so `f(x,y,z)` agrees with chained application. Store supplied-argument state and dispatch only when arity is reached. Currying is useful beyond this notation.

### 31.05.26
- (#representation #decision) Store explicit application nodes from parsing onward. A reducer stack alone loses application structure after execution, which prevents later inspection and code reuse.
- (#resolution #proposal) Perform name resolution in designated opcodes. Raw calculus operations would then require explicit resolution when they need names.
- (#syntax #proposal) Change `~expr` to `:expr`. Separate operators with whitespace and allow colons at identifier edges. An empty label may abbreviate a label carrying `^`.
- (#syntax #decision) Keep block and labeled forms as suffixes. Reject independent block-expression forms because the primary/postfix separation makes expression boundaries clearer.

### 04.06.26
- (#module #decision) Separate module discovery from the effects that modules have on the VM. Discovery must not determine language semantics by itself.
- (#vm #proposal) Model state as reducer plus immutable environment frames and persistent scope graphs. Separate special forms from state so their semantic behavior can be replaced.
- (#function #proposal) Unify native and user callables as arity, immutable supplied arguments, and implementation. The implementation is either a native pointer or a term. Non-callable application raises an error.
- (#evaluation #proposal) Each callee selects its next argument mode: delta receives terms, native operations define policy, and user parameters declare policy. Eager mode evaluates to a value, while term mode preserves code.
- (#syntax #proposal) Consider a standalone `do ... end` block list instead of implicit block currying. Raw tree applications still use reducer rules, with rule 3 for shape and intrinsics for payload inspection.

### 05.06.26
- (#syntax #decision) Permit newline statement separation with explicit continuation and delimiter-sensitive suppression. Newlines after continuation tokens or inside relevant grouped forms do not terminate statements.
- (#compilation #decision) Compile through parsing and semantic analysis. Resolve names/modules and generate cell opcodes with arity and evaluation policy. The pass knows module roots, entrypoint, and built-ins.
- (#implementation #proposal) Consider a C+Lua implementation instead of Zig.

### 06.06.26
- (#function #question) Clarify arity counting: distinguish positional and labeled arguments, or count only positional arguments and treat labels separately.
- (#syntax #proposal) Lower parsed forms to literals, names/operators, lists, and applications. Preserve tags for prefix/infix/grouping distinctions that left-associative desugaring would otherwise erase.
- (#compilation #question) Define exactly what specialized IR does and where its semantic boundary lies.

### 11.06.26
- (#bootstrap #proposal) Launch pure v0 bootstrap code with entry, initialization, and initial arguments. Bootstrap discovers sources, resolves names, checks semantics, handles closures, and lowers code to v0. The exact sequence remains open.
- (#opcode #proposal) A byte string in function position names a native operation from a separate registry. Encoding recognizes known heads and emits opcode leaves, while the same string in data position remains ordinary bytes.
- (#opcode #proposal) Native opcode states accumulate arguments and dispatch at fixed arity. Count named arguments as parameters too. Every executable application has a binary `APPLY` cell, textually written `{$}`.
- (#function #proposal) The current `{fn}/3` design takes parameters, body, and `with`. It temporarily extends the environment and restores it after execution. This draft explicitly chooses dynamic scope for unresolved identifiers.
- (#host #proposal) Initially build host operations into the substrate. Eventually pass sensitive authority explicitly through `initial_term`, because globally nameable intrinsics provide forgeable ambient authority.
- (#module #proposal) The host/build layer supplies candidate sources, while bootstrap decides their meaning. Source discovery therefore remains separate from module semantics.

### 14.06.26
- [ ] (#representation #proposal) Revisit byte encoding.

### 15.06.26
- (#machine #proposal) An explicitly unapproved LLM draft proposes extended CESK with callee-first evaluation, lexical closures, a semantic store, pending arguments, and callee-selected evaluation modes. Source: artifacts/20260613-120000/v0_cesk_draft.md.
- (#machine #proposal) `{fn}` would force arguments, while `{form}` would capture unevaluated code with its environment. Stable boxes would need a store. An immutable first version could use the CEK subset.
- (#machine #proposal) A companion review favors lambda authoring plus triage reflection and borrows ZINC's argument spine and `GRAB` for currying. Despite its decision-summary heading, it is candidate research. Source: artifacts/20260613-120000/v0_basis_review.md.

### 17.06.26
- (#opcode #proposal) Continue exploring `^ [opcode_payload]` and the three-input `{fn}` constructor. These shapes are proposed interfaces, not settled machine semantics.
- [ ] (#implementation #proposal) Implement a minimal machine and literal dispatch first, then application activation and initial terms, then one-file bootstrap. Resolve closures and terms before static lexical resolution, modules, and recursive/mutable bindings.

### 09.07.26
- (#layers #decision) During restoration, establish Layers 0–4 in `v0_layers.md` as the settled boundary. Prefer this account over conflicting CESK drafts. Restating Layer 5 remains pending. Source: artifacts/20260711-120000/v0_layers.md, restoration dated 09.07.2026 in retained history.
- (#opcode #decision) Reapprove native opcode families instead of magic-delta encodings. This replaces the March convention rather than retroactively making that convention a native-family design.
- (#syntax #decision) Set the intended syntax seam to nameful tree terms in cells with spans in a host table keyed by cell index. Reject the draft's source-tree handle boundary as an unintended LLM assumption.
- (#resolution #decision) Confirm lexical scope and retract the 11 June dynamic-scope note. Specialization accepts closed terms, resolves free names to natives or errors, and permits value injection only through application.
- (#representation #decision) Retain `APPLY` from parsing. Applications are structurally inspectable as code but shapeless as values. The machine owns dispatch, with triage reduction as one case, replacing the reducer-outer-loop model.
- (#bootstrap #decision) Use `{load_file}`, `{parse_term}`, and `{specialize}` as backbone intrinsics. Isolate effects in loading. Omit capability machinery from v0.
- (#module #decision) Remove executable `GLOBAL`. Lower top-level and cross-unit references to binder application: a module is a function of imports. A future global pool could optimize linking without changing semantics.
- (#recursion #decision) Derive recursion instead of adding `{rec}`. Strict-lambda stand-ins demonstrate self-passing, library fixpoints, and mutual recursion through function lists. Reported cost is one extra application per recursive call and about twice the stack use.
- (#recursion #experiment) A call-by-name stand-in supports plain Y through `{form}`: it preserves `f(f)` until callee demand. Mutual recursion shares one recursive function list. Waiting remains the Layer 0 reference mechanism.
- (#sequencing #question) Compare pure bind-discard lowering with machine execution of body lists. The first adds closures/frames. The second needs a BLOCK continuation and correct final-statement handling to preserve tail calls.
- (#tailcall #proposal) Define proper tail calls through an invariant: retain a frame only when a sub-result is needed. Require native trampolining without machine re-entry. Control bounds would not bound heap growth before reclamation.

### 10.07.26
- (#syntax #decision) Statements, multiple parameters, and `do!` sugar belong to v0 rather than requiring v1. `do!` is syntax and never a runtime opcode.
- (#compilation #decision) Specialization lowers source forms, resolves names, and creates executable cells and opcode objects. Opcode state identifies kind and transition globally, so separate family identity is redundant.
- (#opcode #decision) Opcode values remain stem-shaped at rest. The completing application dispatches immediately, so no saturated opcode fork becomes observable. Remove `{fn}` from v0 syntax in favor of `do!` construction.
- (#function #proposal) `do! fn: [x, y, ...] do ... end with: ...` and a short `do! fun:` with implicit `it` remain recognition-text candidates. Neither form is an approved syntax contract.
- (#parser #evidence) Current parsing produces host `source_tree_t`, and `bytecode_source_encode` creates cells afterward. The 9 July claim did not describe current implementation. Runtime `{parse_term}` representation and span ownership/keying remain open.
- (#machine #decision) Only Layers 0–4 are settled. `CLOSURE`, `TERM`, environment chains, forcing, continuations, and exact `GRAB` behavior remain candidates. Draft vocabulary must not become semantics without a ruling.
- (#sequencing #proposal) Refine pure sequencing: a one-statement body is the base case, and longer bodies apply a function over the remaining body to the first statement. `_` is an ordinary unused binder. The earlier tail-call trace used assistant extrapolation, not an existing ruling.
- (#machine #question) The minimal-machine dialogue leaves function representation, stored arguments, lexical binding representation, forcing, state, ZINC application rules, effect order, tail-call invariants, and sequencing unresolved. Source: artifacts/20260613-120000/minimal-machine.md, started 10.07.2026.
- (#opcode #proposal) July research models opcode protocols as state graphs whose transitions can depend on arguments. Static protocol recognition versus runtime dispatch remains a crux. Its recommendations are research, not authority. Source: artifacts/20260322-120000/claude_opcode_do_research_2026-07-10.md, conversation dated 10.07.2026 and imported 11.07.2026.
