We
use descriptive names, inline description and examples to minimize the ambiguity.

```
w64 = ;; a fruit of eight bytes, little-endian

a-i64 = [[{:i64}, ...], w64]
;; Example: -42 == [[{:i64}, [], 0, 0], ~{d6 ff ff ff ff ff ff ff}]
;; TODO: exact integer specification, with operations and stuff

TODO: a-f64

a-string = [[{:str}, ...], w64+]
;; Example: u8{Hi 🌍}
;; [[~{3a 73 74 72}, [], 0, 0],
;;  ~{48 00 00 00 00 00 00 00},  ;; H
;;  ~{69 00 00 00 00 00 00 00},  ;; i
;;  ~{20 00 00 00 00 00 00 00},  ;; space
;;  ~{0d f3 01 00 00 00 00 00}]  ;; 🌍
;; TODO: exact string specification, with operations and stuff
;; TODO: lowering of vf syntax could select different trees, probably this should be controlled
;; by parse_term options

TODO: other things
```
