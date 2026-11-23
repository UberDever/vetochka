| case | form | intended | base | cas, sequential | cas, simultaneous binders | cas |
|---|---|---|---|---|---|---|
| A1 parameter bound by application | plain | {1} | ✓ | ✓ | ✓ | ✓ |
| A2 binder clauses in order | plain | ~[{1}, {1}] | ✓ | ✓ | ✓ | ✓ |
| A3 nested lambdas, distinct identities | plain | ~[{1}, {2}] | ✓ | ✓ | ✓ | ✓ |
| A4 same spelling, different identities | plain | ~[{1}, {2}] | ✓ | ✓ | ✓ | ✓ |
| A5 closure keeps its captured value | plain | {1} | ✓ | ✓ | ✓ | ✓ |
| B1 closed lambda passed and called | plain | {3} | ✓ | ✓ | ✓ | ✓ |
| B2 compose: inserted lambda redeclares the same identity | plain | {7} | ✓ | ✓ | ✓ | ✓ |
| B3 twice: one inserted lambda used twice | plain | ~[{w}, ~[{w}, {0}]] | ✓ | ✓ | ✓ | ✓ |
| B4 fix: self-application | plain | [{s}, {s}, {s}] | ✓ | ✓ | ✓ | ✓ |
| B5 partial application holds a lambda, later args reach it | plain | {5} | ✓ | ✓ | ✓ | ✓ |
| B6 lambda applied to itself | plain | {1} | ✓ | ✓ | ✓ | ✓ |
| C1 unique identifier held as data | plain | hole x | ✓ | ✓ | ✓ | ✓ |
| C2 binder built around a hole binds it | data | {9} | ✓ | ✓ | ✓ | ✓ |
| C3 hole passed through a function, then bound | data | {9} | ✓ | ✓ | ✓ | ✓ |
| C4 a different hole escapes the binder | data | hole y | ✓ | ✓ | ✓ | ✓ |
| C5 running a hole | plain | error: unbound | ✓ | ✓ | ✓ | ✓ |
| C6 hole inside a function value, bound where the value is used | data | ~[{9}, {1}] | ✓ | ✓ | ✓ | ✓ |
| D1 mk: header declares a computed identifier | data | {hello} | ✓ | ✓ | ✓ | ✓ |
| D2 binder clause declares a computed identifier | data | {4} | ✓ | ✓ | ✓ | ✓ |
| D3 reflective re-entry | plain | [_, {z}] | ✓ | ✓ | ✓ | ✓ |
| D4 two generated declarations of one identity, nested | plain | (report) | {2} | {2} | {2} | {2} |
| E1 inspect a partial application | plain | (report) | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id... |
| E2 take a lambda apart and rebuild it | plain | ~[{r}, {1}] | ✓ | ✗ [{r}, [{:id}], {a}, ~[{:private}, ~{08}]] | ✓ | ✓ |
| E3 rebuild a lambda through a curried helper \[h, c] [[{:lambda}], ~, h, c] | plain | ~[{r}, {1}] | ✓ | ✗ [{r}, [{:id}], {a}, ~[{:private}, ~{08}]] | ✗ [{r}, [{:id}], {a}, ~[{:private}, ~{08}]] | ✓ |
| F1 arm mentions its unset name | plain | error: not bound | ✗ [{u}, [{:id}], {l}, ~[{:private}, ~{02}]] | ✓ | ✓ | ✓ |
| F2 unset name fed back into the same lambda | plain | error: not bound | ✗ ~[{p}, {p}] | ✓ | ✓ | ✓ |
| F3 identifier taken out of a lambda's header and fed back into it | plain | ~[hole l, {p}] | ✗ ~[{p}, {p}] | ✓ | ✓ | ✓ |
| G1 closures returned from activations | plain | [~[{1}, {x}], ~[{2}, {y}]] | ✓ | ✓ | ✓ | ✓ |
| G2 mutual recursion | plain | {f} | ✓ | ✓ | ✓ | ✓ |
| G3 deep recursion (length 300) | plain | 300 s | ✓ | ✓ | ✓ | ✓ |
| G4 factorial 3 and Fibonacci 6 | plain | 6 s, 8 s | ✓ | ✓ | ✓ | ✓ |
| H1 code kept in the store, read back and run | plain | {5} | ✓ | ✓ | ✓ | ✓ |
| H2 hole read from the store after its binder bound | data | hole x | ✓ | ✓ | ✓ | ✓ |
| H3 store written in one activation, read in another | plain | {7} | ✓ | ✓ | ✓ | ✓ |

cas failures: none
