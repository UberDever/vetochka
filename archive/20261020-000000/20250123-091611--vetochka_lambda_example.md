# Artifact: lambda/opcode syntax example

- Original path: `project/vetochka_lambda_example.md`
- Historical context date: 2026-02-15
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Vetochka Lambda Example

```lisp
:
    lambda : ^ (params) : ^ (closed-overs) : ^ (mutables) nil
        seq 
            print s{Hello, world!}
            print s{From vetochka!}
    nil
```
