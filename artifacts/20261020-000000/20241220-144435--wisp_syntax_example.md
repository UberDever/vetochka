# Artifact: WISP syntax example

- Original path: `project/wisp_syntax_example.md`
- Historical context date: 2026-02-12
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

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
