### 29.07.2026
- ✅ Experiment: alternatives to lambda abstraction in a triage runtime.

#### Question

Triage makes every runtime program a structurally inspectable tree. Could a different
execution model provide human-level names, lexical scope, abstraction and conventional
vf code more naturally than lambda calculus?

The requirements were initially entangled. They are separate:

1. **Program representation and intensionality:** executable programs remain ordinary
   runtime trees which programs can inspect and construct.
2. **Abstraction:** a runtime object suspends an open computation and declares how
   external inputs enter it.
3. **Binding:** occurrences refer to the intended inputs or captured dependencies.
4. **Human names:** programmers can organize and understand code using stable labels.
5. **State:** computations may retain mutable state across interactions.

Triage already solves the first item. It does not by itself provide the input interface,
binding discipline or state model.

Pure compilation from lambda to `SKI`/`BCKW` preserves extensional behaviour, but erases
the binder and usually the abstraction boundary. This is acceptable for execution, but
not as the canonical representation if runtime code should recognize and manipulate
abstractions as abstractions.

#### Models considered

##### Closure/environment machines

A conventional closure is:

```text
closure = code + captured lexical environment
```

The CAM and ordinary CEK/CESK implementations use this shape. Source names normally
become environment slots, projections or resolved binding identities. This provides
lexical scope and first-class callable values, but the implementation closure does not
necessarily retain an inspectable source-level abstraction.

An environment or register machine does not independently solve abstraction. It only
answers where bound values live and how occurrences access them. An abstraction still
has to package executable structure with an input interface and captured context.

##### Runtime lexical identity

A human name and a binding identity must be distinct:

```text
display label: "x"
binding identity: scope token / resolved identifier
```

Otherwise runtime-generated code can be captured accidentally by a same-spelled binder.
Nominal calculi model this with fresh atoms, freshness, swapping and atom-abstraction.
Racket syntax objects use symbols plus scope sets. They are different formal models, but
both demonstrate the relevant engineering rule: spelling is metadata; lexical identity
comes from unforgeable scope information.

This is not an alternative execution model. It is a principled implementation of
binding and hygiene when scoped syntax survives or is constructed at runtime. Extra
nominal primitives are unnecessary unless programs must explicitly open and rebuild
abstractions; then fresh-scope introduction or a safe `open abstraction` protocol may
be needed.

##### Pattern/rewrite abstraction

Pure Pattern Calculus represents an abstraction approximately as:

```text
pattern ->{bound variables} body
```

Application matches an argument against the pattern and instantiates the body. Lambda
is the special case of a one-variable pattern. This keeps the binder interface and body
inside the runtime term and naturally supports destructuring.

For triage this is a real non-lambda candidate:

```text
abstraction = pattern with holes + replacement using those holes
```

However, it does not eliminate binding; it generalizes it. It also introduces matching
semantics, overlap, rule selection, failure and potentially nontrivial pattern
compilation. This is useful when structural rewriting is itself the desired programming
model, but it is not a simpler basis for ordinary functional/imperative vf code.

Arbitrary first-class rewriting was also considered more generally. A scalable rewrite
runtime needs explicit rewrite loci, matching strategy, overlap resolution, scheduling,
identity and effects. Immutable compiled rule bundles and indexed local rewriting could
make such a machine practical, as in rewriting frameworks, but this becomes a
meta-runtime rather than a minimal extension of triage.

##### Stack/concatenative execution

A stack machine can remove names from local value access:

```text
program tree : Stack -> Stack
```

Inputs and outputs are described by a stack effect. Capturing a value means constructing
a new tree which places the value on the stack before executing another tree. Triage
does not need Joy-style quotation: every program already is runtime tree data, and a
subtree may simply move between data and control positions.

Names would still be needed for program organization:

```text
human name -> stable word identity -> executable tree
```

This is Forth-like dictionary binding, not local lexical binding. Local values remain
anonymous stack positions. Early resolution to stable word references would be
preferable to repeated string lookup; explicit late binding could remain available.

The model is coherent:

```text
names identify programs
stack positions identify values
trees represent executable abstractions
```

It was rejected as Vetochka's primary model. It makes the machine smaller at the cost of
making conventional higher-level code semantically alien. Larger functions require
stack shuffling or an extensive vocabulary of structural combinators. Adding convenient
local named values recreates an environment and loses the model's principal distinction.
It would therefore sit poorly beneath vf imperative/functional syntax and confuse the
programmer about the actual dataflow.

##### Message passing and actors

Message passing is a genuinely different model:

```text
identity + behaviour + message -> new behaviour + result/messages
```

It can avoid callable abstraction as the universal organizing concept and provides a
clean account of persistent identity and isolated state. It does not, however, solve
ordinary local abstraction. Actor behaviours are commonly still expressed using
functions, patterns or closures.

A full actor runtime additionally requires mailboxes, scheduling, delivery ordering,
failure semantics and usually supervision. This is too much complexity merely to avoid
lambda abstraction.

Actors remain an attractive orthogonal state model:

```text
lambda + triage   local computation and abstraction
actors/agents     persistent identity and isolated state
```

A later experiment could consider synchronous or cooperatively scheduled agents whose
message handler returns the next behaviour. That should not affect this experiment's
choice of local computation model.

##### Interaction nets and graph reduction

Interaction nets make local graph interaction fundamental. They offer locality,
parallelism and strong confluence properties, but are harder to reason about and program
directly. Exposing sharing and graph identity also complicates triage's simple tree
semantics. They may inform an implementation substrate, but are not a small, clear
extension for vf semantics.

##### First-class environments and contexts

REBOL makes words context-bearing runtime values and permits explicit rebinding of code
blocks. Kernel makes environments first-class and derives applicative functions from
more general operatives. These systems show that runtime names and binding can remain
observable and programmable.

They also make environment selection, rebinding or evaluation part of ordinary runtime
semantics. That power is unnecessary for the current goal and would substantially
enlarge the semantic nucleus.

##### Lambda, SF and tree calculus

`lambda-SF` and tree calculus belong to the same intensional research programme.
`lambda-SF` retains lambda abstraction and extends factorisation so it can be inspected;
tree calculus instead replaces the special `S`/`F` atoms with uniform tree structure.
Tree calculus is described as arising directly from SF-calculus; `lambda-SF` is better
understood as a sibling development rather than the single direct ancestor.

Triage provides the uniform structural eliminator, but a raw subtree does not inherently
say that it is an abstraction or where its inputs enter. That interface must still be
represented by a runtime convention or constructor.

#### Synthesis

Lexical scope does not logically force lambda syntax. Processes may bind channel names,
logic languages may bind through unification, graph languages may expose input ports,
and stack languages may address anonymous positions.

But if the requirements are all of:

- conventional higher-order functions;
- lexical names;
- runtime-created callable values;
- captured lexical dependencies;
- clear correspondence with ordinary functional/imperative source;
- runtime-visible program structure;

then the minimal semantic package is lambda-like:

```text
binding interface + body + captured lexical context
```

It may be encoded as a closure, graph, object or combinator term, but removing this
package requires dropping or changing at least one requirement. A Turing machine,
register VM or raw rewrite engine does not provide a deeper solution; it merely moves
human abstraction into a layer above the machine.

The main alternatives escape lambda by changing the programming model:

- stack effects replace local names;
- patterns replace parameter binding with generalized matching;
- messages replace invocation with interaction;
- ports replace lexical occurrences with graph wiring.

Each is valid, but none is both simpler and a better semantic match for vf.

#### Decision

- ✅ Retain **lambda calculus + triage** as the primary computation model: effectively
  lambda-triage.
- Lambda abstraction remains a recognizable runtime tree form rather than existing only
  as frontend sugar compiled irreversibly to combinators.
- Triage supplies uniform representation, construction and structural inspection.
- Lexical binding supplies the abstraction's input and capture discipline.
- Human spelling is metadata distinct from resolved lexical identity.
- An implementation may lower or compile abstractions for execution, but that lowering is
  not their canonical intensional identity.
- Do not adopt stack execution, generic first-class rewriting, message passing or
  interaction nets as the universal execution model.
- Keep actor-shaped persistent state as a separate possible experiment, not as a
  replacement for local lambda abstraction.

#### Open details

- Exact runtime representation of an abstraction and its captured context.
- Which parts of an abstraction are exposed to triage/lenses: parameter interface, body,
  capture information and/or environment.
- Whether scope identities remain opaque or programs can safely open and reconstruct
  abstractions.
- Exact application and partial-application semantics.
- How local imperative mutation lowers without conflating ordinary variables with
  persistent actors.
- Whether actors/agents are worth adding later for shared or long-lived state.

#### Research references

- Barry Jay, [Tree Calculus](https://github.com/barry-jay-personal/tree-calculus)
- Barry Jay et al., [`lambda-SF`](https://www.sciencedirect.com/science/article/pii/S1571066116300913)
- Barry Jay and Delia Kesner, [Pure Pattern Calculus](https://hal.science/hal-00229331/document)
- Andrew Pitts, [Nominal Logic](https://www.cl.cam.ac.uk/~amp12/papers/nomlfo/nomlfo-draft.pdf)
- [Racket syntax model](https://docs.racket-lang.org/reference/syntax-model.html)
- Manfred von Thun, [Joy](https://www.complang.tuwien.ac.at/anton/euroforth/ef01/thomas01a.pdf)
- Slava Pestov, [Factor quotations and lexical variables](https://factorcode.org/littledan/dls.pdf)
- [REBOL words and contexts](https://www.rebol.com/r3/docs/datatypes/word.html)
- John Shutt, [Kernel](https://ftp.cs.wpi.edu/pub/techreports/pdf/05-07.pdf)
- Yves Lafont, [Interaction Nets](https://dl.acm.org/doi/pdf/10.1145/96709.96718)
