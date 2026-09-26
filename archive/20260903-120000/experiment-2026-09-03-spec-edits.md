# Experiment 2026-09-03: spec edits

Status: experiment, not ruling (user, 2026-09-24). An agent allowed to imagine made these
uncommitted edits to `docs/new_spec/`. They are disregarded as spec changes and recorded here
only as hypotheses. See `tasks/20260911-112058/picture.md` from the same run.

## Notes

- `02_syntax.md`, metadata: claims piths must elide the metadata slot. Unruled; depends on the pith definition.
- `02_syntax.md`, `{@}`: restates it as an ordinary inert node type with a pith, instead of
  "not part of the syntax". Unruled.
- `02_syntax.md`, piths: replaces "TODO: add more" with a list of missing piths. The list is the
  agent's inventory, not a ruling.
- `03_v0.md`, let: marks the `$let` sketch superseded by `let[...] do ... end`, and asks whether
  `let` belongs in core at all. Unruled.
- `03_v0.md`, `$form`: adds `$form` as a named open core form. Unruled.

## Diff

```diff
diff --git a/docs/new_spec/02_syntax.md b/docs/new_spec/02_syntax.md
index 99629a5..42a04d5 100644
--- a/docs/new_spec/02_syntax.md
+++ b/docs/new_spec/02_syntax.md
@@ -137,9 +137,12 @@ Spacing before a loose postfix is immaterial (`$fn:` and `$ fn:` are the same).
 ## Rewrite rules
 
 TODO: metadata list after the node type: `[{node}, [version, line, file, ...], ...payload]`.
+Piths must elide that slot, otherwise a pith-rebuilt node cannot equal a hand-written one;
+see [pith section](#pith).
 
-To support intensionality, syntax above is lowered into simpler terms, representable by the same syntax — with one
-exception: `{@}` marks application and isn't part of the syntax, only notation for the cells to come: `f(x) -> {@} f x`.
+To support intensionality, syntax above is lowered into simpler terms, representable by the same syntax.
+`{@}` marks application: `f(x) -> {@} f x`. It has no source spelling, but it is an ordinary inert node type
+like every other one here — data, observable through its pith, and never executed on its own.
 
 ```text
 1. x                    -> [{:id}, {x}]
@@ -185,8 +188,17 @@ There are following node types:
 3. `nyad2`
     + Represents `~[x, y]` with 2 children, a fork
     + Pith: `~[x, y]`
-TODO: add more
-    
+TODO: add more. Missing piths, each one a blank spot, not an omission:
+
+- `{@}` application marker (list-shaped; its pith must rebuild an equal `{@}` node)
+- `[{:id}, {name}]` — as a node, distinct from what an identifier in demand position dereferences to
+- `[{:label}, ...]`, `[{:block}, ...]`, `[{:annot}, ...]`, `[{:prefix}, ...]`, `[{:infix}, ...]`,
+  `[{:selector}, ...]` — lists, so possibly nothing beyond list piths; that "possibly" is the open part
+- integer literals and byte literals
+- code closures (declarators + body syntax, no env slot)
+- opcode states (selected opcode + inputs as written)
+- the metadata slot, which every pith above must skip
+
 
 `Pith` is a "view" into the node that is generated on demand when the node enters
  [`rule 3` family rule of `v0`](#triage-calculus). On more detail see [pith section](#pith).
diff --git a/docs/new_spec/03_v0.md b/docs/new_spec/03_v0.md
index 6c0fc1b..a626661 100644
--- a/docs/new_spec/03_v0.md
+++ b/docs/new_spec/03_v0.md
@@ -128,19 +128,25 @@ $fn: [] do {stuff} end
 
 2. **Let binding**  TODO:
 
-Syntax:
+SUPERSEDED: the `$let: x is: <expr> <do-end-node>` sketch below is dead. The ruled bootstrap form is
 
 ```
-$let: <param-identifier> is: <expr> <do-end-node>
+let[x = 2, y := 10, z <- foo()] do ... end
 ```
 
-This opcode mainly simplifies low-level `v0` code and improves readability.
+— one form only, terminated by the loose block (`in:` is unnecessary and would not parse, since label
+payloads are tight). Clause kinds are infix operators: `=` plain bind, `<~` recursive (routed through
+`$form`), extensible. Clauses lower to `[{:infix}, op, id, expr]` list entries, so their shape is
+grammar-checked. Scoping is sequential; a list-shaped clause is a knot.
 
-Semantics:
+OPEN: whether `let` belongs here at all. It may be a vmeta form, or an inert `$` form consumed at build
+time, rather than a core opcode. Not ruled. It is also blocked on `$form`'s shape.
 
-Example:
+Old sketch, kept only so the supersession is legible:
 
 ```
+$let: <param-identifier> is: <expr> <do-end-node>
+
 $let: x is: 42 do
 $let: y is: 69 do
 $fn: [] do 
@@ -149,6 +155,12 @@ end end end
 ;; -> 69
 ```
 
+3. **`$form`** TODO:
+
+OPEN: the raw-input form. `$fn` evaluates its `to:` inputs; `$form` is the opcode that retains an input
+raw, in continuity with the historical `{form}`. It is named and motivated — `let`'s `<~` clause kind
+routes through it — but its exact shape is unspecified and subject to change.
+
 ## v0 CESK
 
 TODO: binder discipline — see `tasks/20260830-165615/binders.md`.
```
