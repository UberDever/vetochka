# Module Name Clash Example

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
