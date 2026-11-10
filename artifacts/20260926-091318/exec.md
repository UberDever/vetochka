# Pure-triage `exec` experiment

`exec-core.vf` is the interpreter as one closed core term: nyads and applications.
`exec_scratchpad.py` contains its readable construction and executable checks.
Python expands temporary lambda abbreviations into triage combinators. It does
not interpret the quoted source program on behalf of `exec`.

Run:

```sh
python3 tasks/20260926-091318/exec_scratchpad.py
```

## Contract

`exec` consumes encoded pure-triage syntax and returns its encoded outer result.
It preserves unevaluated children. It repeatedly reduces the demanded positions,
rather than performing one step or normalizing the whole tree.

This is a prototype, not an implementation of v0 names, literals, bindings, blocks,
or native effects. The quotation format is temporary. It does not settle
`parse_term` output or the application pith.

## Quotation

The four ordinary combinators `Q_LEAF`, `Q_STEM`, `Q_FORK`, and `Q_APP` use a
Scott encoding: each selects one of four handlers and supplies its children.
All children are themselves encoded syntax.

```text
Q_LEAF(l)(s)(f)(a)       ->* l
Q_STEM(p)(l)(s)(f)(a)    ->* s(p)
Q_FORK(p)(q)(l)(s)(f)(a) ->* f(p)(q)
Q_APP(p)(q)(l)(s)(f)(a)  ->* a(p)(q)
```

`Q_APP(p)(q)` constructs an application description. It does not apply `p` to `q`.
The Python `quote` helper performs parser-side encoding. The Python `read_code`
helper observes that encoding for assertions; it does not execute represented calls.

## Evaluator

These equations describe the generated interpreter. Their arguments denote
represented syntax, and the helper names abbreviate the algorithm.

```text
head([])       = []
head(~[p])     = ~[p]
head(~[p, q])  = ~[p, q]
head(f(x))     = apply(head(f), x)

apply([], x)       = ~[x]
apply(~[p], x)     = ~[p, x]
apply(~[p, q], x)  = dispatch(head(p), q, x)

dispatch([], q, x)        = head(q)
dispatch(~[u], q, x)      = head(u(x)(q(x)))
dispatch(~[w, v], q, x)   = select(head(x), w, v, q)

select([], w, v, q)       = head(w)
select(~[u], w, v, q)     = head(v(u))
select(~[u, z], w, v, q)  = head(q(u)(z))
```

Thus `exec(Q(t))` produces the encoding of `head(t)`. Recursion uses ordinary
self-application: `EXEC = WORKER(WORKER)`. The interpreter has no free names or
host callbacks.

The native harness extends the earlier scratchpad only to expose a computed
left child of a function fork before selecting its reduction rule. It does not
force native operands. Scott handlers let the interpreter consume a computed
encoding through application, without requiring rule 3 to inspect a pending
native application.

## Checks

The checks cover rules 0a, 0b, 1, 2, 3a, 3b, and 3c, plus demand for the function
discriminator and rule 3's operand root. Divergent terms survive in discarded
operands and returned children. Quotation round trips preserve pending calls.
Executing the divergent term exhausts a bounded run.

The existing tagging checks also cover the shared model. No C runtime tests or
native parser integration are included.
