# Module Syntax Sugar Example

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
