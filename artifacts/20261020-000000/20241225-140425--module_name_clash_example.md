# Artifact: module name-clash example

- Original path: `project/module_name_clash_example.md`
- Historical context date: 2024-12-25
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

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
