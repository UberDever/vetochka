# CLOSURE, environments, and lambda encoding

This records the layout selected in (19.11.26, 15), (19.11.26, 16), and (19.11.26, 17), with the return rule from (19.11.26, 18) and the terminology and application updates from (19.11.26, 19). Uppercase `CLOSURE(...)` denotes a runtime-backed object. The examples use kind-only tag heads and v0 tree notation for their contents.

## Runtime layout

Ref kind `110` denotes a `CLOSURE`. Its headerless record stores two short refs, in order: the tree ref and the environment ref. At `w = 2` the record occupies four bytes. The collector follows both fields.

```text
Buffer offset  Bytes        Meaning
00000000       06 01        CLOSURE ref to 0x20
00000020       01 02 01 03  tree ref to 0x40, environment ref to 0x60
```

An environment is a private persistent list of name–value forks. `~` is the empty environment. Extension puts a new binding at the front and shares the previous list. Lookup starts at the front. A binding may hold an ordinary v0 value or a `CLOSURE` that preserves the stored tree's own context.

## Lambda tree

The fields are the `:lambda` tag head, the ordered parameter-name list, and the body. While arguments remain to be supplied, application produces a `CLOSURE` carrying the lambda tree and the extended environment. The lambda in that state has a trailing private fork with the remaining-parameter count. Application produces updated state and shares the body. In-place rewrites require an explicit exception. A name-bearing identifier has the `:id` tag head and a name fruit as its payload.

For example, after supplying `{a}` for `{x}` and `{b}` for `{y}`:

```text
CLOSURE(
  [
    [{:lambda}],
    [{x}, {y}, {z}],
    ~[[[{:id}], {x}], ~[[[{:id}], {y}], [[{:id}], {z}]]],
    ~[{:private}, ~{01}]
  ],
  [~[{y}, {b}], ~[{x}, {a}]]
)
```

Here `01` is an example of a private encoding for one remaining parameter. The next application supplies `{z}` and reaches zero. Execution then enters the body with the extended bindings:

```text
CLOSURE(
  ~[[[{:id}], {x}], ~[[[{:id}], {y}], [[{:id}], {z}]]],
  [~[{z}, {c}], ~[{y}, {b}], ~[{x}, {a}]]
)
```

Triage walks the lambda's v0 tree as encoded, including its parameter list and the whole private fork. The private fork's bytes follow the policy at (19.11.26, 14). See (19.11.26, 24).

## Other users and results

A binder evaluates an ANF complex binding expression, then extends the environment for its body. Triage extends the environment for the selected arm, with extracted children retaining their original contexts. Application extends the environment with the supplied parameters. Binder and triage tree field layouts remain open.

When one of these computations returns an arbitrary tree, `CLOSURE` carries the context needed to interpret it. For example, an ordinary fork can be returned with a pending substitution:

```text
CLOSURE(
  ~[[[{:id}], {x}], {b}],
  [~[{x}, {a}]]
)
```

Through that context, triage sees `~[{a}, {b}]`. If the result is already a `CLOSURE`, return it directly and discard any outer wrapper that the return would otherwise add. Its tree and environment travel together through the returning scope.

## Sequential binder example

Place each subsequent binder in the preceding binder's body. Each binding expression below is an ANF application with an atomic callee and an atomic argument. The example uses the provisional binder field layout.

```text
[
  [{:binder}],
  [~[{x}, [[{$}], [[{:lambda}], [{p}], [[{:id}], {p}]], {a}]]],
  [
    [{:binder}],
    [~[{y}, [[{$}], [[{:lambda}], [{q}], [[{:id}], {q}]], [[{:id}], {x}]]]],
    [[{:id}], {y}]
  ]
]
```

The first application yields `{a}`, which the outer binder binds to `{x}`. The inner binding expression uses `{x}` to produce the value for `{y}`.

An active call can still accumulate newly consumed bindings in a temporary flat segment as a ZINC-style optimization. When a result needs those bindings after the segment ends, it needs a persistent environment. The batching and materialization rules remain open. See (19.11.26, 0) and the [ZINC report, §3.4](https://xavierleroy.org/publi/ZINC.pdf).
