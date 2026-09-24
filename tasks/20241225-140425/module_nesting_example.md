# Artifact: module nesting example

- Original path: `project/module_nesting_example.md`
- Historical context date: 2024-12-25
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Module Nesting Example

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
