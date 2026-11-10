# Artifact: internal tagging approach sketch

- Original path: `project/internal_tagging_approach.md`
- Historical context date: 2025-01-23
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Internal Tagging Approach

```elixir
3_val = ^(^^^)
tag_int = 3_val

to_string_int = <convert int to list of bytes>

tag = \val \tag ^ val tag
untag = <untag impl>

3 = tag 3_val tag_int
plus = <plus implementation> # plus unwraps the values and adds them, then wraps again

print = \x evalcall x
print (to_string_int 3) # list of bytes that interpreter actually understands
```
