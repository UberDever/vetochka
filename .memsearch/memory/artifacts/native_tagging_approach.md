# Artifact: native tagging approach sketch

- Original path: `project/native_tagging_approach.md`
- Historical context date: 2025-01-23
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Native Tagging Approach

```elixir
# 3 is predefined
# tag_int is predefined
tag = <predefined in the interpreter> # allows to construct some value and tag it
untag = <predefined in the interpreter> # allows to get 3_val
get_tag = <predefined in the interpreter> # allows to get tag_int

to_string_int = <could still be encoded in the calculus, or could be native also>
plus = <plus implementation> # this implementation can be native also

print = \x evalcall x
print 3 # since interpreter knows about the tags it can convert the values accordingly
```
