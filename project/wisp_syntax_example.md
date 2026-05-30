# WISP Syntax Example

```
(a ((b c))
    d (e f)
    g
)
```

as

```lisp
a : : b c
  . d : e f
  . g
; or
a
  :
    b c
  . d
  e f
  . g
```

Parens are still allowed from time to time for simplification:

```
a : (b c)
  . d (e f)
  . g
```
