# Deferred substitution that behaves exactly as rewriting

User rule (23.11.26, 0): substitution is deferred, but it must behave exactly as if trees were
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

## Prior art

Read directly (fetched 23.11.26):

- Matt Might, "Writing an interpreter, CESK-style", https://matt.might.net/articles/cesk-machines
  - State `(exp, ρ, σ, κ)`. `ρ : Var ⇀ Addr`, `σ : Addr ⇀ Value`; lookup is `σ(ρ(v))`.
  - Values: void, integers, booleans, `clo(lam, ρ)`, `cont(κ)`. `A(lam, ρ, σ) = clo(lam, ρ)`.
  - Call: evaluate callee and arguments atomically, then
    `applyproc(clo((λ (v1..vn) body), ρ), vals, σ, κ) = (body, ρ[vi ↦ ai], σ[ai ↦ vali], κ)`
    with fresh addresses `ai`.
  - `let`: push `letk(v, body, ρ, κ)`; returning an atomic value applies the continuation,
    binding `v` at a fresh address in the saved `ρ`.
  - `set!` overwrites `σ(ρ(v))`; `letrec` extends `ρ` first and evaluates the right-hand sides
    in the extended environment.
  - An environment is only ever applied to the expression under evaluation, and a closure's
    environment only to its own body. Environments are never composed, and no value has a free
    variable waiting for a binder outside it: a closure captures an environment covering all of
    its free variables, and programs cannot build code.
- Matt Might, "A-Normal Form", https://matt.might.net/articles/a-normalization/
  - Atomic expressions terminate and have no side effects, control effects, or errors; lambdas
    are atomic. Complex expressions are let-bound or in tail position.
  - The Flanagan et al. normalizer: `normalize M k` with `normalize-name` naming every
    non-atomic result with a fresh `gensym` let-binding. Our v0 compiler does the same, with
    `unique` as `gensym`.

From web-search summaries (primary texts not read):
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
closure's body. Vetochka values are trees that can hold holes, and a program can place such a
tree inside a binder or lambda that it builds around it (code is data). When such
code runs, the pending substitutions of its parts must compose, and that is what explicit
substitution calculi provide: a closure `M[s]` under a further substitution `t` becomes
`M[s ∘ t]`, where `(s ∘ t)(x) = s(x)[t]` for `x` in `s`, and `t(x)` otherwise; substitution
stops at binders that redeclare an identity it replaces.

## Proposal (agent)

- The reference machine rewrites trees for real (copying substitution), so it behaves as
  rewriting by construction.
- The production machine implements explicit-substitution composition in place of the
  evaluator's places and fallback, and is cross-tested against the reference machine.

## Shadowing (confirmed by the user, (23.11.26, 0))

Rewriting in place stops at any lambda or binder that redeclares the identity being substituted
(capture avoidance). `fix` needs it: inside a call of `w`, `w`'s own tree appears as a value and
declares `self` again; substituting the outer `self` into it would replace the inner binder's
declaration and occurrences.
