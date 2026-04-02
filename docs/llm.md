# LLM Research Notes

## Opcode tree shape

```
unsaturated stem:  ^ [magic_N  payload...]        = delta1( delta2(value(magic_N), payload) )
saturated fork:    ^ [magic_N  payload...]  args   = delta2( delta2(value(magic_N), payload), args )
```

- `magic_N` is a unique native integer per opcode — both sentinel and opcode identity
- Fixed magic position: `fork.left.left` — O(1) VM check
- VM dispatch: `is_fork && is_fork(left) && is_value0(left.left)` → switch on `left.left.integer`
- User extensibility via `call` opcode carrying a native function pointer in payload

## BCKW combinator derivation in pure triage-calculus

### Rules

```
1.   ^ ^ x y         = x
2.   ^ (^ x) y z     = (x z)(y z)
3a.  ^ (^ w x) y ^       = w
3b.  ^ (^ w x) y (^ u)   = x u
3c.  ^ (^ w x) y (^ u v) = y u v
```

### K

```
K = ^ ^

K x y = ^ ^ x y = x    (rule 1)
```

### I

```
I = ^ (^ ^ ^) ^

Parsing: ^ (^ ^ ^) ^ is a fork.
  Left child = (^ ^ ^) = fork(^, ^), so w=^, x=^.
  Right child (y) = ^.

I ^       = ^ (^ ^ ^) ^ ^       → rule 3a: result = w = ^       ✓
I (^ u)   = ^ (^ ^ ^) ^ (^ u)   → rule 3b: result = x u = ^ u   ✓
I (^ u v) = ^ (^ ^ ^) ^ (^ u v) → rule 3c: result = y u v = ^ u v ✓
```

### S (inline — rule 2 IS S)

Rule 2 directly implements S:

```
^ (^ x) y z = (x z)(y z)

So S x y = ^ (^ x) y, used inline.
S x y z = ^ (^ x) y z = (x z)(y z)    (rule 2)
```

S has no simple standalone closed form because producing the stem `^ (^ x)` from
an arbitrary `x` requires wrapping x in two layers of `^`, which requires case-splitting
on x's shape (rules 3a/3b/3c), leading to infinite regress.

### B

Goal: `B f g x = f (g x)`

**Deriving what `B f` must be:**

We need `B f g x = f (g x)`. The last step uses rule 2:

```
^ (^ (K f)) g x
  rule 2: a = K f = ^ ^ f, b = g, c = x
= (K f x)(g x)
= f (g x)           (rule 1: K f x = f)  ✓
```

So `B f g = ^ (^ (K f)) g`, meaning `B f = ^ (^ (K f)) = ^ (^ (^ ^ f))`.

**Deriving B itself:**

We need a closed term B such that `B f = ^ (^ (^ ^ f))` for all f.

The last step of `B f` must produce `^ (^ (^ ^ f))`. Using rule 2:

```
^ (^ a) b f = (a f)(b f)

Set a = K ^ = ^ ^ ^, b = K = ^ ^:
^ (^ (^ ^ ^)) (^ ^) f
  rule 2: a = ^ ^ ^, b = ^ ^, c = f
= (^ ^ ^ f)(^ ^ f)
= (K ^ f)(K f)
= ^ (K f)           (K ^ f = ^, rule 1; then ^ applied to K f)
= ^ (^ ^ f)         (K f = ^ ^ f)
```

This gives `^ (^ ^ f)` — one `^` short of what we need. We need `^ (^ (^ ^ f))`.

Apply `^` to the result: we need one more wrapping. Use rule 2 again on the outside:

```
^ (^ (^ (^ ^ ^))) (^ (^ ^)) f
  rule 2: a = ^ (^ ^ ^), b = ^ (^ ^), c = f
= (^ (^ ^ ^) f)(^ (^ ^) f)
```

Now evaluate each part:
- `^ (^ ^ ^) f`: this is I applied to f (since I = ^ (^ ^ ^) ^... wait, I = ^ (^ ^ ^) ^, not ^ (^ ^ ^)).
  `^ (^ ^ ^) f` is a fork (value) — it's the term I without its second argument.
  Actually `^ (^ ^ ^)` is a stem, and `^ (^ ^ ^) f` is a fork (value, stops here).

This approach is getting complicated. Let me try rule 3 directly.

**Using rule 3 to build B:**

We want `B f = ^ (^ (^ ^ f))`. Case-split on f:

```
f = ^:       B ^ = ^ (^ (^ ^ ^)) = ^ (^ K)
f = ^ u:     B (^ u) = ^ (^ (^ ^ (^ u)))
f = ^ u v:   B (^ u v) = ^ (^ (^ ^ (^ u v)))
```

Using rule 3 pattern `^ (^ w x_i) y`:
- `f = ^`     → result = w
- `f = ^ u`   → result = x_i u
- `f = ^ u v` → result = y u v

Set:
- `w = ^ (^ (^ ^ ^))` (result for f=^)
- `x_i u = ^ (^ (^ ^ (^ u)))` so `x_i = ?`
- `y u v = ^ (^ (^ ^ (^ u v)))` so `y = ?`

For `x_i`: `x_i u = ^ (^ (^ ^ (^ u)))`.

`^ u` is a stem. `^ ^ (^ u) = K (^ u)` is a fork. `^ (K (^ u))` is a stem. `^ (^ (K (^ u)))` is a stem.

Using rule 3b on x_i: `^ (^ w2 x_i2) y2 (^ u) = x_i2 u`.
Set `x_i2 = ^`: result = `^ u`. But we want `^ (^ (^ ^ (^ u)))`, not `^ u`.

We need to wrap `^ u` three more times. Each wrapping requires another rule 3b application.

**The pattern:** to wrap an arbitrary term `t` in `^` (producing `^ t`), we need:

```
wrap t:
  t = ^:       result = ^ ^         (a specific constant)
  t = ^ u:     result = ^ (^ u)     (rule 3b with x=^: ^ (^ w ^) y (^ u) = ^ u... gives ^ u not ^ (^ u))
```

There is no single rule that uniformly wraps any term in `^`. This confirms that B, as a standalone closed term, requires an infinite tower of case-splits — it cannot be expressed as a finite tree in pure triage-calculus.

**Resolution:** B is used *inline* by constructing `^ (^ (^ ^ f)) g` directly in the bytecode/tree, not as a standalone combinator. The combinator laws hold as equational identities for reasoning, but B is not a runtime term.

### Verified inline usages

```
seq x y = K y x:
  ^ ^ y x = y    (rule 1)  ✓

B f g x (inline as ^ (^ (^ ^ f)) g x):
  ^ (^ (^ ^ f)) g x
    rule 2: a = ^ ^ f = K f, b = g, c = x
  = (K f x)(g x)
  = f (g x)    (rule 1)  ✓

W f x (inline as ^ (^ f) f x):
  ^ (^ f) f x
    rule 2: a = f, b = f, c = x
  = (f x)(f x)  ✓

S K I ^ (using S inline):
  ^ (^ K) I ^
    rule 2: a = K = ^ ^, b = I = ^ (^ ^ ^) ^, c = ^
  = (K ^)(I ^)
  = (^ ^ ^)(^ (^ ^ ^) ^ ^)
  = (^ ^ ^)(^)              (I ^ = ^, from I derivation)
  = ^ ^ ^ ^
  = ^                       (rule 1: K ^ ^ = ^)  ✓
  (S K I is identity, so S K I ^ = ^)
```
