# Requirements

- A Language, based on `tree-calculus`, facilitating it's ideas to make a language that can compile to `C`
- `C` output should be human-readable and maintainable
- The language should have high metaprogramming capabilites, such that it will behave as a wrapper to `C` code
- It should **not** provide a runtime of any sort, at least in form of builtin dependency


# Ideas

- **Didn't work out** <a name="tokenizer">Tokenizer</a> will parse anything, tokenizing by whitespace. *Except* for stuff enclosed in `{}`, this
could be considered a `string`, a series of bytes encoded in `utf-8`.
    - Better use more complex tokenization to support various levels of sugar

- Scopes:
    * Declared as `scope [name: string | none] do ... end`
    * Named scopes are `modules` and unnamed ones are alternative form of `let-bindings`
    * Inside: series of sugared bindings of the form
        ```elixir
        scope do
            a = 10
            b = 20
            ^
        end
        # equivalent to
        scope do
            let a = 10 in
            let b = 20 in
            ^ # `^` is a false value
        end
        ```
    * Ending expression of the scope is final value of the expression. If no
        expression is provided, false/nullish idk value is returned (maybe 
        do an analysis before? not for interpreter...)
    * No recursive and mutually-recursive bindings are currently considered
        - They are achieved via combinators and stuff
    * This notion of scope is not very mathematical, rather functional-flawored.
        This is because scopes are not first-class and are not part of the language
        internal representation (i.e. `tree-calculus` trees).
        But this notion seems functional enough, also easy to implement.
        The more rigorous implementation would consider scopes as a first-class entities
        with ability to combine and unpack them in several ways. And I'm afraid there
        would still be a problem of integration with filesystem
        

- Modules can be done pretty easily:
    * Create a `root.tree` in the project
    * ~~In case this root is empty -- all paths will be relative to this root~~ The set of modules
      `M` is composed of every module found in every file that can be found recursively from the `root.tree` directory
    * Bad decision: ~~Otherwise -- this root contains code that can be executed on setup (this is just
        a room for extension, i don't think this will be actually used)~~.
        Better to use `root.tree` as anchor for module location and nothing more.
        Otherwise, we blend together solutions (build time code execution and module location)
        which is not orthogonal.
    * Interpreter accepts:
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
    * Side note: Interpreter accepts either (1) source file to interpret or (2) path to `root.tree` and
        `entry point` source file. The former doesn't allow for `use` constructs because
        there is no explicit module locator prefix and I don't want to provide implicit heuristics.
        `root.tree` stands out as a way for linking the interpreter and project
        file system. This way is done through a file because it is "conventional?".
        Other way is to use shell variable like `ROOT_TREE` which can be used in the interpreter
        in similar fashion. I prefer the file tho.
    * Module name is a string. Since all the modules that are accessible from the `root.tree` are assumed to
        be in programmer's control, it is their responsibility to come up with unique name for each module.
    * Module is just a named scope. Reference to named scope can be done in every other
        scope, even in separate file. Project is just a certain collection of files,
        each containing scopes and their usages (inside the scope). Project is defined by `root.tree` in the project directory
    * All modules created as follows:
        ```elixir
        scope {something} do
            ...
        end

        ```
    * ~~Nesting of modules can be added as a syntax sugar, but I don't see if it's necessary. Right now modules
        considered to be a linear sequence of declarations in the file.~~
        Nesting of modules does nothing to their names, instead, it just brings outer definitions to the scope of
        the inner module
        ```elixir
        # Modules can be nested
        scope {A} do
            a = ...
            scope {B} do
                b = a
            end
        end

        # Since named scopes are considered modules, they are always exported with
        # every each of their declarations being available
        # If you need to 'hide' implementation of a module, you can do this
        scope do
            get_impl = ...
            # Here, KV is exported anyway since it is named, even if it is nested inside anon scope
            scope {KV} do
                get = get_impl ...
                set = ... get_impl ...
            end
        end
        ```
    * All modules can be used as follows:
        ```elixir
        use {stuff} do
            ...
        end

        # modules can be referenced anywhere (in the same file multiply in any place)
        use {other} do
            use {some/thing} do
                ...
            end
        end
        ```
    * `use` construct can enclose any expression, and since `scope` is an expression on itself, `use`
        can appear on top level with unrestricted embedding depth.
    * `use` brings in top-level declarations (i.e. let bindings) into the scope of the expression
        following `in`.
        This can be a little clunky because it semantically can be considered
        as qualified prefix and every use of such prefix is then
        `use {some/path.tree} in func foo bar` rather than `path.func foo bar`.
        But this really simplifies module system.
        If you have a name clash you can alias the clashing things yourself.
        ```elixir
        scope {KV} do
            get = ...
        end
        scope {State} do
            get = ...
        end
        scope {User} do
            kv_get = use {KV} in get
            state_get = use {State} in get
            ...
        end
        ```
    * The `module` can be described as a syntax sugar. It acts as a "statement" that doesn't return
        anything. This makes language less consistent, but much pleasant to work with.
        ```elixir
        scope do
            module {KV} do
                get = ...
            end
            use {KV} in get
        end
        # Translates to
        scope do
            _ = scope {KV} do
                get = ...
                ^
            end
            use {KV} in get
        end
        ```
    * If module is defined at top-level, implicit clause is created
        ```elixir
        module {A} do end
        module {B} do end
        module {C} do end
        use {Bool} in true
        # Translates to
        scope do
        _ = scope {A} do ^ end
        _ = scope {B} do ^ end
        _ = scope {C} do ^ end
        use {Bool} in true
        end
        ```

- Let-bindings
    * Since we have scopes, I decided to reuse this construct for let bindings also.
        Honestly, this feels natural from a semantical point of view, need to see it in practice
    * Therefore:
        `let a = 10; b = 15; c = 20 in a + b + c; end`
        =>
        `scope do a = 10; b = 15; c = 20; a + b + c; end`

- To accomplish project goal, that is the **mighty preprocessor** the following could be done:
    - Make a functional language based on `tree-calculus` (with sugar and statements in some form)
    - Take embedded code in `C` and transpile it to `tree-calculus`
    - Write a transformations you want, that can also do evaluation of the resulted code on the go
    - Transpile `tree-calculus` code back to `C` and enjoy your system language
This is ridiculously hard if done in full swing, but for some subset of `C` it can be pretty easy. Also, some properties
of `tree-calculus` can help to build a convenient `ir` (tagging, evaluation without interpretation).

- On fast (relatively) and compact (relatively) representation:
    * Firstly, need to somewhat use cache locality => store tree/commands in the contiguous memory
    * Idk how to represent `tree-calculus` trees as commands or something, so stick to the tree
    * Node of the tree should be as small as possible => stick to a number (64-bit).
        Side note: actually, since we are storing *and evaluating* (adding nodes) of the tree, 
        nodes can't be implicitly linked by their position in the memory, because
        this would require copying them around. This means that we should store link information 
        explicitly, in the nodes themselves => this strips the restriction of uniform node size.
        Which means that node can be of any size and can be actually VLE encoded.
        But 64 bit nodes and single array is pretty simple implementation, so I stick to it.
    * Node can be encoded as follows:
        | Node type           | 0..1 | 2..32 | 33..63 |
        |---------------------|------|-------|--------|
        | Tree node           | 00   | lhs   | rhs    |
        | Application node    | 01   | lhs   | rhs    |
        | Intrinsic node      | 1\*  | \*    | \*     |

        For `tree node`: lhs and rhs should be relative (in memory)
        offsets with reserved value `2**31 - 1` which indicates absence of the node.
        For `intrinsic node`: This node can store any data that is intrinsic for a
        interpreter i.e. built-in functions and variables.

# TODO

- [ ] Write a language
    - [x] Make a grammar
    - [x] Make a simple tokenizer for the language
        - [x] Should every token be a string? Or we can decide if token is a `number` on tokenizer level?
            - Encoding to known structures (lists, numbers, strings) should be done in the interpreter 
        - [x] Write a couple of smoke tests for tokenizer
        - [ ] Add source information to tokens
        - [ ] Add unicode delta as special symbol
        - [ ] COMMENTS??? use `#`
        - [ ] [Unimportant] Rewrite the tokenizer to be more powerful
    - [ ] Sugar
        - [ ] Module clauses
        - [ ] Use statement in module
        - [x] Lists
            - [x] Parsing
            - [ ] Encoding
            - [ ] Decoding
        - [ ] Scopes and let bindings
            - [x] Parsing
            - [x] Described
            - [ ] Semantics
            - [ ] Encoding
            - [ ] Decoding
        - [ ] Lambda abstractions
            - [ ] Parsing
            - [ ] Described
            - [ ] Semantics
            - [ ] Encoding
            - [ ] Decoding
    - [ ] Make evaluator
    - [ ] Write all different stdliby necessary stuff
        - [ ] Modules?
            - [x] Described system
        - [ ] State/IO
    - [ ] .... Tooling?
        - [ ] Repl
            - [x] Architecture
            - [ ] Support sugar (scopes and uses)
