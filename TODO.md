# Requirements

- A Language, based on `tree-calculus`, facilitating it's ideas to make a language that can compile to `C`
- `C` output should be human-readable and maintainable
- The language should have high metaprogramming capabilites, such that it will behave as a wrapper to `C` code
- It should **not** provide a runtime of any sort, at least in form of builtin dependency


# Ideas

- **Didn't work out** <a name="tokenizer">Tokenizer</a> will parse anything, tokenizing by whitespace. *Except* for stuff enclosed in `{}`, this
could be considered a `string`, a series of bytes encoded in `utf-8`.
    - Better use more complex tokenization to support various levels of sugar


- To accomplish project goal, that is the **mighty preprocessor** the following could be done:
    - Make a functional language based on `tree-calculus` (with sugar and statements in some form)
    - Take embedded code in `C` and transpile it to `tree-calculus`
    - Write a transformations you want, that can also do evaluation of the resulted code on the go
    - Transpile `tree-calculus` code back to `C` and enjoy your system language
This is ridiculously hard if done in full swing, but for some subset of `C` it can be pretty easy. Also, some properties
of `tree-calculus` can help to build a convenient `ir` (tagging, evaluation without interpretation).

# TODO

- [ ] Write a language
    - [ ] Make a simple tokenizer for the language
        - [x] Should every token be a string? Or we can decide if token is a `number` on tokenizer level?
            - Encoding to known structures (lists, numbers, strings) should be done in the interpreter 
        - [x] Write a couple of smoke tests for tokenizer
    - [ ] Make an encoder, that will encode tokens as trees (just python recursively embedded lists of zeros). This
    will give an ability to describe any data with quotation on the tokenizer level
        - [ ] Make boolean encoding
        - [ ] Make list encoding
        - [ ] Make number encoding
        - [ ] Make string encoding as utf-8 constants
    - [ ] Sugar
        - [ ] Let bindings
        - [ ] Lambda abstractions
    - [ ] Make evaluator
    - [ ] Write all different stdliby necessary stuff
    - [ ] .... Tooling?