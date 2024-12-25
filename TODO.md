# Requirements

- A Language, based on `tree-calculus`, facilitating it's ideas to make a language that can compile to `C`
- `C` output should be human-readable and maintainable
- The language should have high metaprogramming capabilites, such that it will behave as a wrapper to `C` code
- It should **not** provide a runtime of any sort, at least in form of builtin dependency


# Ideas

- **Didn't work out** <a name="tokenizer">Tokenizer</a> will parse anything, tokenizing by whitespace. *Except* for stuff enclosed in `{}`, this
could be considered a `string`, a series of bytes encoded in `utf-8`.
    - Better use more complex tokenization to support various levels of sugar

- Modules can be done pretty easily:
    * Create a `root.tree` in the project
    * In case this root is empty -- all paths will be relative to this root
    * Bad decision: ~~Otherwise -- this root contains code that can be executed on setup (this is just
        a room for extension, i don't think this will be actually used)~~.
        Better to use `root.tree` as anchor for module location and nothing more.
        Otherwise, we blend together solutions (build time code execution and module location)
        which is not orthogonal.
    * Interpreter accepts either (1) source file to interpret or (2) path to `root.tree` and
        "entry point" source file. The former doesn't allow for `use` constructs because
        there is no explicit module locator prefix and I don't want to provide implicit heuristics.
        Side note: `root.tree` stands out as a way for linking the interpreter and project
        file system. This way is done through a file because it is "conventional?".
        Other way is to use shell variable like `ROOT_TREE` which can be used in the interpreter
        in similar fashion. I prefer the file tho.
    * All modules created as follows:
        ```ocaml
        module {something} in
            ...
        end

        -- Modules can be nested
        module {A} in
            module {B} in
                -- This effectivelly means combination
                -- of names (i.e. {A/B})
            end
        end

        -- Module without explicit name gets the name of the full path to the file
        -- containing the module
        module in
            The path to this module is i.e. {/home/uber/dev/project/src/file.tree}
        end
        ```
    * All modules can be used as follows:
        ```ocaml
        use {src/stuff.tree} in
            ...
        end

        -- modules can be referenced anywhere (in the same file multiply in any place)
        use {src/other.tree} in
            use {src/something.tree} in
                ...
            end
        end
        ```
    * `use` brings in top-level declarations (i.e. let bindings) into scope of `in ... end`
        This can be a little clunky because it semantically can be considered
        as qualified prefix and every use of such prefix is then
        `use {some/path.tree} in func foo bar end` rather than `path.func foo bar`.
        But this really simplifies module system.

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
        offsets with reserved value `max(2**31)` which indicates absence of the node.
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
        - [ ] COMMENTS??? `use --`
    - [ ] Make a parser to `ir` that supports sugary constructs. We will lower the language from here
        - [x] Learn how to parse left-associative application :/
    - [ ] Make an encoder, that will encode tokens as trees. This
    will give an ability to describe any data with quotation on the tokenizer level
        - [x] Make encoding functions written in plain text `^ (^^) ...` and then parse it using
this parser and map to `executable tree` format (optionally efficient)
        - [ ] Make boolean encoding
        - [ ] Make list encoding
        - [ ] Make number encoding
        - [ ] Make string encoding as utf-8 constants
    - [ ] Sugar
        - [x] Lists
        - [ ] Let bindings
        - [ ] Lambda abstractions
    - [ ] Make evaluator
    - [ ] Write all different stdliby necessary stuff
        - [ ] Modules?
            - [x] Described system
    - [ ] .... Tooling?
