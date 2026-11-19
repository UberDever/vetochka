# Identifier handling: historical comparison

## Current clarification

- (user) Closure application substitutes bound identifiers as if rewriting the term in place. Only this substitution accounts for the earlier word "dereferencing". Other identifiers remain data because no substitution replaced them.
- (agent) This removes the proposed intrinsic-side ambient lookup step. Supplied compound terms need no implicit evaluation to resolve names already replaced by closure substitution.

## Historical evidence and limits

- December 2024: scopes and bindings disappear before tree-calculus representation; bindings compute on demand. This supports an early separation between binding machinery and resulting trees, but does not establish the current substitution rule. Source: artifacts/20261020-000000/20260201-183604--project_history.md, December scope account.
- June 2026: the fn draft explicitly uses dynamic lookup for unresolved identifiers. July retracts that direction in favor of lexical scope. Source: the same history artifact, June fn account; devlog (09.07.26, 3).
- July 2026: accepted Model B uses nameful code with lookup through captured E on demand. This is not the current replacement-only rule for identifiers that remain in supplied trees. Source: artifacts/20260717-120000/lexical-reference-models.md and devlog (17.07.26, 2). The source heading says July 11, while the log notes conflicting dates.
- August 2026: binders.md explicitly states "No substitution" and preserves machine term/environment pairs. Escaping data is instead divided into embedded values, thunks retaining behavior, and open syntax that rebinds at its destination. This supports distinguishing embedded values from open syntax, but its destination-rebinding behavior is not identical to the user's current clarification. Sources: artifacts/20241225-140425/binders.md, devlog (30.08.26, 8-10).
- October 2026: closure operands remain raw, bindings force computations, and closure application tentatively returns a body with bindings installed. Observation follows bound identifiers while preserving unbound names. These are compatible motivations but do not alone establish substitution-only handling. Sources: artifacts/20260529-103914/2026-10-23-anf.md and 2026-10-26.md.
- November 5: the user explicitly says lambda substitution constructs terms without forcing them and let stages forcing. This is the strongest direct predecessor. Source: artifacts/20260529-103915/2026-11-05.md, Corrections to the synthetic assessment; devlog (05.11.26, 3).

## Assessment

- (agent) The current clarification continues the recent substitution/staging direction. It is not an unchanged rule across the entire history.
- (agent) Under this direction, an intrinsic consumes the supplied tree without a second name-resolution pass. The earlier intrinsic-input question assumed too much ambient lookup.
- (agent) Closure origin alone does not imply every identifier disappears. Identifiers not covered by a substitution remain data, including those in syntax inserted as an argument. Subsequent substitution must respect binder scope; this observation does not select a hygiene mechanism.
- (agent) Earlier rules make execution of an unbound identifier an error while allowing its inspection as data. The current clarification about supplied argument trees does not by itself revoke that execution rule.

No specification or evaluator implementation was changed.
