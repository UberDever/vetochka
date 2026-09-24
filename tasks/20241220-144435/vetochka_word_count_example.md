# Artifact: word count syntax experiment

- Original path: `project/vetochka_word_count_example.md`
- Historical context date: 2026-02-16
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

# Vetochka Word Count Example

```lisp
;; any opcode can have a luxury to accept any syntax, this is a very powerful idea
;; unit = ^
;; all closed ones must be provided by intrinsics, I think
;; .lambda body is desugared in the sequence of `.seq` since it is a list
;; TODO: idk how to do functions with multiple arguments
;; TODO: imperative for is awkward
.lambda [
  closed= [<= and or expr for concat length hashmap-set hashmap-get hashmap-keys print] 
  params= [is-letter to-lower count-words]
] 
  : .set is-letter ;; .lambda param is implicit
      : .lambda [c] : expr [[{a} <= c and c <= {z}] or [{A} <= c and c <= {Z}]]
    .set to-lower .lambda [c]
      : .if : expr {A} <= c and c <= {Z}
          expr c - {A} + {a}
          c
    .set count-words .lambda [closed= [is-letter to-lower] params= [text] locals= [counts current]]
      : .set counts : hashmap []
        .set current : {}
        for [c text]
          : .if (is-letter c)
              .set current : concat [current (to-lower c)]
              : .if (> (length current) 0)
                  .seq hashmap-set [counts (current (+ 1 (hashmap-get [counts current])))]
                    .set current {}
        .if : (expr (length current) > 0)
          .seq hashmap-set [counts (current (+ 1 (hashmap-get [counts current])))]
          unit
        counts
      .lambda [locals= [text counts] closed=[count-words]]
        : .set text {Hello world hello test world}
          .set counts (count-words text)
          for [key (hashmap-keys [counts])]
            print (concat key (concat {: } (hashmap-get counts key)))
        unit
  unit
```
