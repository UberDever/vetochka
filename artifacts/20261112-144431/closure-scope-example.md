# Required closure scope behavior

Source: user example, 2026-11-12. This records required behavior, not an implementation result.

```vf
(\[x] do x(5) end)(\[y] do [[{:id}, ...], {x}] #| a desugared identifier, unbound and free |# end)
=>
#| with env {x: \[y] do x end} |# x(5)
=>
#| with env that was captured on substitution {} |# x
=>
#| terminate with unbound parameter |#
```

- (user) The free identifier x in the supplied closure retains the empty captured context shown in the example. The outer parameter x does not capture that occurrence.
- (agent interpretation) The bound x in the outer body selects the supplied closure. Applying that closure to 5 reaches its free x, which does not resolve through the caller's x binding. Execution terminates with the unbound-parameter outcome specified by the user.
- (agent correction) Keeping arguments unevaluated during substitution does not prohibit execution of the closure body. The earlier assistant claim that fast calls must avoid body execution was incorrect and is withdrawn. This example does not establish a complete general stepping or return protocol.

No closure representation, physical substitution strategy, or parameter-name retention rule is selected by this example.
