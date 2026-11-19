# Lambda syntax and v0 encoding experiment

These examples pair the `\[…] do … end` form from (12.11.26, 2) with the lambda encoding from (19.11.26, 19). Each v0 block contains a whole tree. Tag heads contain just the kind in these fixtures. The full source metadata fields in `docs/spec/02_v0.md` are a separate part of lowering. `CLOSURE(...)` denotes a runtime-backed value, as in the environment examples at (19.11.26, 15).

## Parameter name, identifier, and literal

```vf
\[x] do x end
```

```text
[[{:lambda}], [{x}], [[{:id}], {x}]]
```

The declaration contains a name fruit. The body contains an identifier whose payload is that same name fruit. A raw byte literal stays a fruit even when its bytes spell the parameter name.

```vf
\[x] do ~{78} end
```

```text
[[{:lambda}], [{x}], ~{78}]
```

## Application in a lambda body

```vf
\[f, x] do f(x) end
```

```text
[
  [{:lambda}],
  [{f}, {x}],
  [[{$}], [[{:id}], {f}], [[{:id}], {x}]]
]
```

The callee and argument are atomic identifier trees. The application is the whole body.

## Multiple parameters and execution progress

```vf
(\[x, y] do ~[x, y] end)(~{61})
```

```text
[
  [{$}],
  [[{:lambda}], [{x}, {y}], ~[[[{:id}], {x}], [[{:id}], {y}]]],
  ~{61}
]
```

After supplying the first argument, the runtime produces:

```text
CLOSURE(
  [
    [{:lambda}],
    [{x}, {y}],
    ~[[[{:id}], {x}], [[{:id}], {y}]],
    ~[{:private}, ~{01}]
  ],
  [~[{x}, ~{61}]]
)
```

The example private byte `01` means one parameter remains. A subsequent application to `~{62}` enters the shared body with both bindings:

```text
CLOSURE(
  ~[[[{:id}], {x}], [[{:id}], {y}]],
  [~[{y}, ~{62}], ~[{x}, ~{61}]]
)
```

The corresponding explicitly nested source form has a different tree shape:

```vf
\[x] do \[y] do ~[x, y] end end
```

```text
[
  [{:lambda}],
  [{x}],
  [[{:lambda}], [{y}], ~[[[{:id}], {x}], [[{:id}], {y}]]]
]
```

Both execute as curried lambdas. These examples preserve the parameter grouping written in the source. The triage decision below applies to both shapes.

## Escaping lambda with an outer binding

```vf
(\[x] do \[y] do x end end)(~{61})
```

```text
[
  [{$}],
  [[{:lambda}], [{x}], [[{:lambda}], [{y}], [[{:id}], {x}]]],
  ~{61}
]
```

The result pairs the inner tree with the retained environment:

```text
CLOSURE(
  [[{:lambda}], [{y}], [[{:id}], {x}]],
  [~[{x}, ~{61}]]
)
```

The identifier tree is unchanged. The binding comes from the environment carried by the result. Applying this value inside a new scope still selects the retained `{x}`.

## Local shadowing

```vf
(\[x] do \[x] do x end end)(~{61})
```

```text
[
  [{$}],
  [[{:lambda}], [{x}], [[{:lambda}], [{x}], [[{:id}], {x}]]],
  ~{61}
]
```

When the returned lambda is later applied to `~{62}`, the runtime enters:

```text
CLOSURE(
  [[{:id}], {x}],
  [~[{x}, ~{62}], ~[{x}, ~{61}]]
)
```

The newest binding supplies `~{62}`. Shadowing follows from adding a binding to this lambda's own environment. The identifier encoding stays the same as in the capture example.

## Empty parameter list

```vf
\[] do ~ end
```

```text
[[{:lambda}], [], ~]
```

This tree fits the field layout. The [retained vf grammar](../20261110-000001/new_spec/02_syntax.md) requires at least one identifier in `closure_head`, so the source form is a syntax probe. Whether `\[]` is accepted and what causes such a lambda to enter its body remain open.

## Findings

- A parameter declaration lowers to a name fruit. An occurrence lowers to a tagged identifier. A raw byte literal uses the same fruit encoding in both syntaxes.
- One lambda tree shape covers a body that is an identifier, a fruit, a fork, an application, or another lambda. A runtime-backed `CLOSURE` carries the environment separately.
- A comma-separated parameter list can stay flat in the tree while execution consumes parameters in unary steps. The explicitly nested spelling retains nested trees.
- Each example uses one expression inside `do … end` and maps it directly to the lambda body. The current `vf` spec is empty, so this is a lowering assumption for the experiment. The encoding of a multi-expression body and the insertion of complete source metadata remain open.

## Decisions after the experiment

- A core lambda requires at least one named parameter. A parameter can go unused, as in `\[unused] do ~ end`. The name keeps the input required to enter the body explicit. Triage follows the same principle with exactly one clause each for leaf, fork, and fruit. See (19.11.26, 22).
- Flat and explicitly nested parameters produce distinct v0 trees. Currying equivalence describes execution. Triage walks the tree as encoded. See (19.11.26, 23) and (19.11.26, 24).
