# Module Use Example

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
