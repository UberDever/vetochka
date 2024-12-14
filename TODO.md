# Requirements

- A Language, based on tree-calculus, facilitating it's ideas to make a language that can compile to `C`
- `C` output should be human-readable and maintainable
- The language should have high metaprogramming capabilites, such that it will behave as a wrapper to `C` code
- It should **not** provide a runtime of any sort, at least in form of builtin dependency

# Ideas

- <a name="tokenizer">Tokenizer</a> will parse anything, tokenizing by whitespace. *Except* for stuff enclosed in `{}`, this
could be considered a `string`, a series of bytes encoded in `utf-8`.

# TODO

- [ ] Make a simple [tokenizer](#tokenizer) for the language
    - [ ] Write a couple of smoke tests for tokenizer