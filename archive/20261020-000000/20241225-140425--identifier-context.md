# Identifier context and declaration

Status: working model confirmed by user; not yet a specification decision.

## Framing

Question: Can `TERM`-associated contextual identifier resolution, dynamic function declarators, and Rule 3 structural observation coexist without accidental capture or identifier-resolution traps?

Constraint: preserve “as above, so below”: constructed identifier/function structure has the same meaning as corresponding source structure. Do not introduce binding-identity references such as `LOCAL`.

## Model

- An identifier is resolved according to bindings present where its containing term was assembled. `TERM` carries that declaration-site lexical binding context rather than any environment active where the term is later received.
- An identifier state is: unbound; bound without a value; or bound with a value. Only bound-with-value dereferences. Unbound and bound-without-value remain raw term data.
- Rule 3 observes the dereferenced value when one exists, otherwise raw term data. It therefore does not raise a name-resolution error.
- `$fn` first forces and validates its complete declarator list under existing context. It then associates its body with a derived environment containing the resulting parameter bindings in declared/no-value state; those bindings shadow same-spelled outer bindings. A completed identifier in declarator position is not then dereferenced by an outer binding of the same spelling.
- A declarator may be constructed from executable code. `$fn: [[{:id}, x]] ...` forces enough of its candidate to obtain an identifier-shaped declarator; if `x` evaluates to `{z}`, result declares `z`. Its body therefore sees newly declared `z`, rather than an outer valued `z`.
- Example intended result:

  ```vetochka
  $fn: [z] do $fn: [x] do $fn: [[{:id}, x]] do z end end to: {z} end to: 69 to: 42
  ```

  returns `42`, rather than turning inner function into `$fn: [69] do 69 end to: 42`.
- Existing constructed function terms placed under names already bound in their receiving context do not capture those bindings through their declarator names.
- `$fn: [name] do [{:id}, name] end` can construct and pass identifier structure for later declaration/instantiation. In a declaration context it remains datum; outside one it follows ordinary contextual resolution.

## Resolution (2026-08-30)

Position is dynamic, not syntactic: an identifier is data while nothing
demands it (subterm of a value, raw retained operand, unsaturated body);
observed when Rule 3 shape-matches it; executed when it enters control.

| | unbound | declared, no value | bound with value |
|---|---|---|---|
| undemanded | datum | datum | datum |
| Rule 3 observes | raw datum | raw datum | observes the value |
| executed | trap (ruled) | unreachable (invariant) | dereference |

Invariant: declared-no-value bindings exist only inside closure datums
awaiting saturation (declarators force under outer context; bindings install
at saturation; body runs only saturated). Future env-touching opcodes must
preserve it; assertable in the machine.
