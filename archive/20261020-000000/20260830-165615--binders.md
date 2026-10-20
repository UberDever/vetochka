# Binders

Status: settled direction (2026-08-30), not yet in spec by choice.

## The axiom

Binders are the sole nonlocality in the language, side effects aside.
Everything else is triage: pure structural trees, where "you inspect exactly
what you wrote" is trivially true because nothing refers anywhere.

- E is a property of binder nodes alone; fragments are pure trees.
- An identifier is a written reference. Observation follows it: bound-with-value
  observes the value, otherwise the raw name. This is not a breach of
  syntax-faithful observation — the reference is what was written.
- Closure pith shows the structural half (declarators, body syntax), no env
  slot. Observation across a binder boundary is re-quotation: rebuilt syntax
  rebinds where it lands. Duality holds everywhere else.

## TERM: machine cell vs value

No substitution means every deferred evaluation pairs a term with an env:
pending operands (callee-first retains operand with E-caller), opcode raw
inputs ($form). These TERMs are machine-internal — K and opcode state — never
observable data.

The only env-carrying value is the closure. **TERM everywhere in the machine,
TERM only at binders in the data.** Rule 3 sees values only, so observation
stays clean and the no-substitution discipline stays total.

## Escaping terms (why TERM-everything died)

Old step-5 wording ("returned as a term with bound E-fn for on-demand
resolution") conflated three intents, each with its own spelling:

1. embed values — build by application: `list(v, 1)` derefs `v`, yields a
   closed pure tree; quasiquote is at most vf sugar over this;
2. keep behavior — return a thunk `$fn: [] do ... end`;
3. keep open syntax — return the literal tree; it rebinds at the destination
   (template feature, not a bug).

Plain `x` as a last body expression executes and derefs; it never needed TERM.
Ambient late binding on escaped terms is deliberately lost.

## Invariants

- Declared-no-value bindings live only inside closure datums awaiting
  saturation; never in scope of running control (declarators force under the
  outer context, bindings install at saturation). Assertable in the machine.
- Executed unbound identifier: uncatchable trap. Observation never errors.

## For the CESK pass

Every deferral rule constructs a machine-TERM; closure construction alone
promotes one to a value; nothing else does.
