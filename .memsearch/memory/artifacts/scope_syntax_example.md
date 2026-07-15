# Artifact: scope syntax example

- Original path: `project/scope_syntax_example.md`
- Historical context date: 2024-12-25
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Scope Syntax Example

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
