# Vetochka v0 execution design

20 November 2026

## 1. Current model

A binder runs one requested operation and binds its result.
A lambda or triage accepts its operand without running it.
Both enter one body expression with lexical bindings.
A body can return a tree that represents another operation.
The v0 force utility repeats binder steps until the outer form is a value.

This model supports code as data with ordinary tree operations.
A program can construct, pass, inspect, and return executable trees.
A binder can run such a tree when the program requests execution.
The design needs no dedicated quotation form.
The experiment demonstrates this for the cases it exercises.

We considered strict lambda and triage calls.
Under strict triage, a second match would run an extracted application before inspecting it.
A binder that continued through every returned application would also run code that a transformer intended to return.
The current model uses lazy operands and one-step binders so a transformer can return its result as a bare tree.

## 2. Tree representation and notation

v0 has three primitive kinds.

- A leaf is ~.
- A fork has two children. For example, ~[{a}, {b}] is a fork.
- A fruit contains bytes. For example, ~{61} and {a} encode the same byte.

The empty list is ~. A nonempty proper list is a sequence of forks whose right spine ends in ~.
For example, [{a}, {b}] means ~[{a}, ~[{b}, ~]].
The two spellings encode the same tree.

Tags identify control forms. The tag head is the first field of the tree.
The examples below show only the kind in the tag head.
The v0 draft also specifies source information and metadata in the tag head.
Programs can build these trees directly at runtime. vf can lower source forms to these trees.
See docs/spec/02_v0.md.

## 3. Applications and ANF

ANF means A-normal form. An executable application has an atomic callee and operand.
Identifiers, callable literals, and ordinary values can serve as atoms.
Use a binder to run a call whose result supplies the callee for a later call.
An ordinary fork can carry an application as a child across an argument boundary.

A {$} application supplies one operand. When the application runs, it resolves the callee first.
Argument lookup uses the caller's lexical context and preserves the resulting tree.
A callee receives a tree even when that tree represents an application or a binder.
The ANF restriction applies to application slots. Ordinary tree construction can contain executable forms.

Might's CESK grammar allows a general expression as a binding RHS.
His normalization article uses a narrower grammar for that position.
The current direction allows a nested binder as an RHS with its own local scope.
The written tree specifies the forcing points.
An automatic rewrite that inserts a binder can change when a computation runs.
Moving a named binding can also change what a generated identifier can find.
See devlog (23.10.26, 3) and (23.10.26, 4).

## 4. Binders and body execution

RHS means the right-hand side of a binding.
The prototype binder has a tag head, a list containing one binding fork, and a body.
The binding fork contains the name on the left and the RHS on the right.

```text
[
  [{:binder}],
  [~[{x}, ~{61}]],
  [[{:id}], {x}]
]
```

When a binder runs, it runs its RHS in the incoming lexical context.
It binds the result and enters the body with that binding.
An RHS can name a stored executable tree. Lookup selects the tree to run.
One request can take several machine transitions.
For example, a call can enter a body that contains more binders.
The outer binder accepts the tree that the call returns.

A lambda body, a triage arm, and a binder body each contain one expression.
Entering a binder expression runs it. A body can also return a tree from lookup or construction.
The continuation receives the returned tree. A later binder can request execution of that tree.

Nested binders express sequencing. A later binding can use an earlier binding.
A binding takes effect after its RHS returns. This is a nonrecursive binding rule.
A binder in an RHS has local scope. A result can carry that context when it escapes.
The recorded multi-binding form lowers to nested single bindings.
Ordinary vf blocks introduce no lexical scope.
See devlog (12.11.26, 10) and (13.11.26, 3).

A lazy argument can remain unused or run more than once.
For example, two binders can run the same stored application twice.
Use one binder when the computation must run once. Reuse its result.

In this example, the first binder stores a call in {next}.
The second binder runs that call and returns {ready}.

```text
[
  [{:binder}],
  [~[{next}, [
    [{$}],
    [[{:lambda}], [{unused}], [[{$}], [[{:lambda}], [{unused}], {ready}], ~]],
    ~
  ]]],
  [
    [{:binder}],
    [~[{result}, [[{:id}], {next}]]],
    [[{:id}], {result}]
  ]
]
```

## 5. Lambdas, environments, and partial application

A lambda tree contains a tag head, an ordered list of parameter names, and a body.
Every lambda has at least one named parameter. The body may leave a parameter unused.

```text
[[{:lambda}], [{x}], [[{:id}], {x}]]
```

A parameter name is a fruit. An identifier is a tagged tree with a name fruit as its payload.
The current prototype uses name-bearing identifiers.

CLOSURE(tree, E) denotes a runtime object. E is the private lexical environment for the tree.
Older records call this object TERM.
A callable uses its retained environment. Its caller supplies the operand in the operand's own lexical context.
The runtime captures a lambda or triage supplied as an operand before it substitutes that operand.
If the runtime defers lookup, an operand with free names also needs to retain its lexical context.

The environment is a persistent list of name–value forks.
The empty environment is ~. Adding a binding puts a fork at the front and shares the previous list.
Lookup uses the first matching name. The runtime can extend or retain the environment in constant time.
Lookup can take time proportional to the length of the list.

A binder, a saturated lambda, or a triage arm can return a tree with its environment.
An ordinary fork or identifier can carry this context.
When a result already carries an environment, that environment takes precedence at the return boundary.
The runtime returns the existing CLOSURE directly. This preserves the lexical meaning of an escaping result.

Applications supply parameters in order. A partial application produces new state and shares the original body.
When the last operand arrives, the runtime enters the body with the accumulated bindings.
A flat parameter list and nested unary lambdas can have the same curried behavior.
Triage sees the distinct v0 trees and preserves the parameter grouping.

The recorded layout uses a trailing private fork for partial application progress.

```text
CLOSURE(
  [
    [{:lambda}], [{x}, {y}],
    ~[[[{:id}], {x}], [[{:id}], {y}]],
    ~[{:private}, ~{01}]
  ],
  [~[{x}, {a}]]
)
```

Here ~{01} illustrates a state with one parameter left.
Triage can inspect the entire private fork and can see substitutions for supplied parameters.
Programs can hold, pass, and print the private state during a run.
The bytes can change and have no stable meaning or persistence beyond RAM.
Relying on private bytes to construct another term makes that use malformed.
The implementation can choose whether to detect this use.
See devlog (19.11.26, 12) through (19.11.26, 24).

## 6. Triage and context

Triage is a first-class callable. A {$} application supplies its subject.
The prototype uses one shared [lhs, rhs] name block and three body expressions in leaf, fork, fruit order.

```text
[
  [{:triage}],
  [{lhs}, {rhs}],
  ~,
  [[{:id}], {lhs}],
  [[{:id}], {lhs}]
]
```

- The leaf arm sets neither name.
- The fork arm sets lhs to the left child and rhs to the right child.
- The fruit arm sets lhs to the whole fruit and leaves rhs unset.

Triage inspects the subject's outer shape with application and binder trees intact.
It enters the selected body in its retained environment with the bindings from the match.
Each extracted child retains its own context.
An unset name shadows an outer binding of the same spelling. The arm can observe it as an identifier tree.

Bound identifiers resolve through their own lexical context in both triage and execution.
Reading a bound identifier can expose a tree or the rest of a list.
This read supplies structure. A binder can request execution of the executable form that the read exposes.
An unbound name remains an identifier when triage inspects it.

The pattern-matching function proposal treats an ordinary lambda parameter as a pattern that binds the whole operand.
Triage would use the same application and body rules with three shape patterns.
The proposal still needs a decision on shared semantics and representation.
See devlog (20.11.26, 3) and (20.11.26, 5).

## 7. The v0 force experiment

The prototype provides one-step binder execution. The force utility is a v0 program.
Lazy triage reads the outer tag. The utility uses fruit-byte equality to compare the tag with known control tags.
The comparison uses ~ and {true} as temporary results. Language truth, equality, and the if intrinsic remain separate design topics.

The utility binds worker(worker) to force. This supplies self and returns a closure waiting for a term.

- On a recognized application, binder, or identifier form, the utility uses a binder to advance the term.
- The worker calls itself with the result.
- On a value, the utility returns it after resolving any remaining name bindings.
- The triage on the predicate result uses ~ as its unused fork fallback.

An ordinary fork is a value even if it contains code.
Forcing repeatedly examines the outer form.
Existing list and lambda forms can carry code across a forcing boundary when the program needs to preserve it.

The seven prototype tests pass. The existing Lua suite passes 25 tests.
The tests include a binder that returns a bare call, later execution of that call exactly once, and a fork containing a call.
They also cover lexical context, a binder extracted through triage, unset match names, and an unbound identifier.
A separate probe builds a binder from an ordinary list and runs it to a fruit.
See artifacts/20261120-172943/force.v0 and devlog (20.11.26, 4) through (20.11.26, 8).

## 8. The triage round-trip bug

Identity triage can rebuild a fork from its bound children.

```text
[
  [{:triage}], [{lhs}, {rhs}],
  ~,
  ~[[[{:id}], {lhs}], [[{:id}], {rhs}]],
  [[{:id}], {lhs}]
]
```

In the prototype, rebuilding a call or binder this way exposes a bug.
Triage can recover the original right child. Dispatch resolves the tag but reads the fields along the raw right spine.
It reads the identifier's fields in place of the application or binder fields.
When the verification supplies the resolved fields, both examples return {ready}.

Execution must use the same lexical bindings for the tag and the fields.
It must preserve each field's retained context when it follows the right spine.
The prototype still contains the faulty field read.
See artifacts/20261120-172943/spike.py and devlog (20.11.26, 7).

## 9. Cells and the machine

The selected cells design uses a growable contiguous buffer and two-byte refs.
The low three bits of each ref identify its kind.
The buffer can move as a unit when it grows. The runtime updates the base.
A short ref uses an unsigned offset from the current base. The initial base is the start of the buffer.
A leaf ref carries no payload. A fork occupies four bytes and stores two refs in a headerless record.
A fruit can occupy the ref itself or an allocation with a ULEB128 length and its bytes.
A known-fruit ref indexes a separate retained registry.

A long ref occupies four bytes. It carries a signed displacement from its own location.
Long refs can chain in either direction. A terminal long ref to a fork supplies the base for that fork's children.
Paths to the same fork must agree on its base.
Runtime refs provide sharing. Language-level references and cyclic structures are a separate design topic.
Evaluation can loop while the represented trees remain finite.

A CLOSURE record also occupies four bytes. Kind 110 locates the record.
The record contains a short tree ref followed by a short environment ref. The collector follows both.
The user selected two-byte refs after comparing widths and allocation locality.
One test placed 6,291,456 forks in 24 MiB at two bytes per ref and 48 MiB at four bytes per ref.
The two-byte layout gave lower traversal times with grouped placement. Timings with interleaving varied by workload.
See devlog (19.11.26, 1) through (19.11.26, 10) and (19.11.26, 19).

CESK names control, environment, store, and continuation.
The machine needs proper tail calls. It retains a continuation only when it needs a sub-result.
A ZINC-inspired proposal collects pending inputs and builds bindings as it consumes them.
Its goals include fast immediate currying and fewer intermediate closure allocations.
The pending-input boundary, exact continuation rules, and allocation guarantee remain open.
See devlog (30.08.26, 14), (13.11.26, 4), and (19.11.26, 0).

## 10. Intrinsics and remaining decisions

An intrinsic has an interpreter-defined operation, an input protocol, and execution behavior.
Intrinsics take their arguments as a list. The named argument encoding remains open.
The current direction gives intrinsics the operands after lexical substitution.
The policy for observing and consuming those operands still needs a precise specification.
See devlog (12.11.26, 8) and (12.11.26, 11) through (12.11.26, 13).

- Partial application. Triage can expose an outer binding for a pending parameter in the prototype.
  A pending parameter should remain an identifier during inspection and extraction, even if an outer binding has the same name.
- Error management. Define malformed control forms, attempts to call ordinary values, and invalid identifier execution.
  Define how triage observes an intrinsic. The prototype errors when force inspects one as a value.
  The prototype also raises an error when a binder tries to run an unbound identifier.
- Identifier identity. Decide whether a generated name fruit can create an executable identifier.
  Name-bearing identifiers support lookup inside a closure by a name fruit supplied at runtime.
  If the runtime requires an issued identity, specify what copying and rebuilding through triage preserves.
  The force utility needs a stopping rule that agrees with binder execution.
  If identical visible trees differ in executability, the utility needs a way to distinguish them.
  See devlog (20.11.26, 2).
- Bootstrap. Specify what the production entry point accepts and returns.
  Ordinary entry in the spike returns a bare application tree. Explicit force runs it.
- Tail calls. A chain of 12 returned calls reached a value. A chain of 128 hit the Python stack limit.
  The real machine needs to reuse its continuation across the force loop.
- Representation. Complete the control layouts and the CESK transitions against these rules.
  Keep the provisional vf spelling, the handwritten example, and the end. proposal in the syntax discussion.
  See artifacts/20261120-172943/force.vf and devlog (20.11.26, 6) through (20.11.26, 8).
