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
