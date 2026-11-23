# Deferred substitution that behaves exactly as rewriting

User rule (22.11.26, 24): substitution is deferred, but it must behave exactly as if trees were
rewritten in place.

## What the throwaway evaluator does now (ad hoc)

- A term is `(tree, env, fallback)`.
- An occurrence is looked up in the term's own `env`.
- Following bound occurrences accumulates "places": the env of every occurrence followed, plus
  each value's fallback. A hole (identifier unbound in its own env) is looked up in those places,
  innermost first. A non-identifier result carries the places as its fallback.
- Declarations are read in their own env only.
- Special cases: a selected case body gets `case.env | parameters` (parameters recovered from the
  header's identities); a binder clause gets `clause.env | earlier clauses`; a partial
  application keeps its header in the defining env.

This is not exactly rewriting:
- the fallback holds whole environments of places, so a hole can take a binding that rewriting
  would never have applied to it;
- capture avoidance happens only when a declaration is read, not at binders that lie between a
  hole and the place that would fill it.
It passes the 30 checks, but it is a reinvention with corner cases.

## Prior art (from web search summaries; primary texts not read, matt.might.net is blocked here)

- Matt Might, "Machine-based interpreters" (CESK): the environment maps variables to addresses,
  the store maps addresses to values; lambdas become closures `(lambda, env)`. Source code is
  fixed; a closure's environment only ever applies to that closure's own body.
  https://matt.might.net/articles/cesk-machines
- Matt Might, "A-Normal Form": atomic vs complex expressions; non-atomic subexpressions are
  let-bound or in tail position; lambdas count as atomic because building a closure has no
  effect. https://matt.might.net/articles/a-normalization/
- Van Horn and Might, "Abstracting Abstract Machines": store-allocated bindings and
  continuations. https://www.ccs.neu.edu/home/dvanhorn/pubs/vanhorn-might-jfp12.pdf
- Biernacka and Danvy, "A Concrete Framework for Environment Machines" (TOCL 2007): calculi of
  explicit substitutions are the middle layer between substitution and environment machines;
  from Curien's calculus of closures they derive Krivine's machine, CEK, and ZINC.
  https://brics.dk/RS/05/15
- Accattoli, Barenbaum, Mazza, "Distilling Abstract Machines" (ICFP 2014): environment machines
  implement the linear substitution calculus; machine steps map to substitution steps or to
  structural congruence, so the machine provably behaves as substitution.
  https://arxiv.org/abs/1406.2370

## Why vetochka needs more than CESK

In CESK the code is fixed: values are closures, and an environment is only ever applied to its
closure's body. Vetochka programs build code from values at run time (code is data). When such
code runs, the pending substitutions of its parts must compose, and that is what explicit
substitution calculi provide: a closure `M[s]` under a further substitution `t` becomes
`M[s ∘ t]`, where `(s ∘ t)(x) = s(x)[t]` for `x` in `s`, and `t(x)` otherwise; substitution
stops at binders that redeclare an identity it replaces.

## Proposal (agent)

- The reference machine rewrites trees for real (copying substitution), so it behaves as
  rewriting by construction.
- The production machine implements explicit-substitution composition in place of the
  evaluator's places and fallback, and is cross-tested against the reference machine.
