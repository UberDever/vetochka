| case | form | intended | base | roles | provenance | fresh | fresh+shadow |
|---|---|---|---|---|---|---|---|
| A1 parameter bound by application | plain | {1} | ✓ | ✓ | ✓ | ✓ | ✓ |
| A2 binder clauses in order | plain | ~[{1}, {1}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| A3 nested lambdas, distinct identities | plain | ~[{1}, {2}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| A4 same spelling, different identities | plain | ~[{1}, {2}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| A5 closure keeps its captured value | plain | {1} | ✓ | ✓ | ✓ | ✓ | ✓ |
| B1 closed lambda passed and called | plain | {3} | ✓ | ✓ | ✓ | ✓ | ✓ |
| B2 compose: inserted lambda redeclares the same identity | plain | {7} | ✓ | ✓ | ✓ | ✓ | ✓ |
| B3 twice: one inserted lambda used twice | plain | ~[{w}, ~[{w}, {0}]] | ✓ | ✓ | ✓ | ✓ | ✓ |
| B4 fix: self-application | plain | [{s}, {s}, {s}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| B5 partial application holds a lambda, later args reach it | plain | {5} | ✓ | ✓ | ✓ | ✓ | ✓ |
| B6 lambda applied to itself | plain | {1} | ✓ | ✓ | ✓ | ✓ | ✓ |
| C1 unique identifier held as data | plain | hole x | ✓ | ✓ | ✓ | ✓ | ✓ |
| C2 binder built around a hole binds it | plain | {9} | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✓ | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 0a}]] |
| C2 binder built around a hole binds it | role | {9} | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| C2 binder built around a hole binds it | data | {9} | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| C3 hole passed through a function, then bound | plain | {9} | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✓ | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 09}]] |
| C3 hole passed through a function, then bound | role | {9} | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| C3 hole passed through a function, then bound | data | {9} | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| C4 a different hole escapes the binder | plain | hole y | ✓ | ✓ | ✓ | ✓ | ✓ |
| C4 a different hole escapes the binder | role | hole y | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| C4 a different hole escapes the binder | data | hole y | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| C5 running a hole | plain | error: unbound | ✓ | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✓ | ✓ |
| C5 running a hole | role | error: unbound | ✗ [{:use}, [{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✗ [{:use}, [{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✗ [{:use}, [{:id}], {x}, ~[{:private}, ~{00 00 80 06}]] | ✗ [{:use}, [{:id}], {x}, ~[{:private}, ~{00 00 80 06}]] |
| C6 hole inside a function value, bound where the value is used | plain | ~[{9}, {1}] | ✗ ~[[[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]], {1}] | ✗ ~[[[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]], {1}] | ✓ | ✓ | ✗ ~[[[{:id}], {x}, ~[{:private}, ~{00 00 80 0d}]], {1}] |
| C6 hole inside a function value, bound where the value is used | role | ~[{9}, {1}] | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| C6 hole inside a function value, bound where the value is used | data | ~[{9}, {1}] | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| D1 mk: header declares a computed identifier | plain | {hello} | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✓ | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 0b}]] |
| D1 mk: header declares a computed identifier | role | {hello} | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| D1 mk: header declares a computed identifier | data | {hello} | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| D2 binder clause declares a computed identifier | plain | {4} | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 00}]] | ✓ | ✓ | ✗ [[{:id}], {x}, ~[{:private}, ~{00 00 80 0e}]] |
| D2 binder clause declares a computed identifier | role | {4} | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| D2 binder clause declares a computed identifier | data | {4} | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| D3 reflective re-entry | plain | [_, {z}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| D4 two generated declarations of one identity, nested | plain | (report) | {2} | {2} | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | {2} |
| D4 two generated declarations of one identity, nested | role | (report) | ✗ error: a declaration must be an identifier | {2} | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| E1 inspect a partial application | plain | (report) | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id}], {a}, ~... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[{:decl}, [{:id}... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id}], {a}, ~... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id}], {a}, ~... | [[{:lambda}], ~[{:private}, ~{01 02}], [[[[{:id}], {a}, ~... |
| E2 take a lambda apart and rebuild it | plain | ~[{r}, {1}] | ✓ | ✓ | ✓ | ✓ | ✓ |
| F1 arm mentions its unset name | plain | ~[{u}, hole l] | ✓ | ✓ | ✓ | ✓ | ✓ |
| F2 unset name fed back into the same lambda | plain | ~[hole l, {p}] | ✗ ~[{p}, {p}] | ✗ ~[{p}, {p}] | ✗ ~[{p}, {p}] | ✓ | ✓ |
| G1 closures returned from activations | plain | [~[{1}, {x}], ~[{2}, {y}]] | ✓ | ✓ | ✓ | ✓ | ✓ |
| G2 mutual recursion | plain | {f} | ✓ | ✓ | ✓ | ✓ | ✓ |
| G3 deep recursion (length 300) | plain | 300 s | ✓ | ✓ | ✓ | ✓ | ✓ |
| G4 factorial 3 and Fibonacci 6 | plain | 6 s, 8 s | ✓ | ✓ | ✓ | ✓ | ✓ |
| H1 code kept in the store, read back and run | plain | {5} | ✓ | ✓ | ✓ | ✓ | ✓ |
| H2 hole read from the store after its binder bound | plain | hole x | ✓ | ✓ | ✓ | ✓ | ✓ |
| H2 hole read from the store after its binder bound | role | hole x | ✗ error: a declaration must be an identifier | ✓ | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier | ✗ error: a declaration must be an identifier |
| H2 hole read from the store after its binder bound | data | hole x | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ | ✓ |
| H3 store written in one activation, read in another | plain | {7} | ✓ | ✓ | ✓ | ✓ | ✓ |
| I1 persist g(g(id)) as plain text, reload in a new run, apply | plain | {7} | ✓ | ✓ | ✗ error: a declaration must be an identifier | ✓ | ✓ |

Passed per variant (all forms): base: 32/47, roles: 32/46, provenance: 36/48, fresh: 38/48, fresh+shadow: 33/47
