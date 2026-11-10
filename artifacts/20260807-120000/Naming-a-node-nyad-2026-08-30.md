## User

what if I call a node a nyad

## Assistant

You can. **`nyad` is a pretty good invented technical noun**, especially if you want to distance the concept from generic graph/tree “nodes.”

Tradeoff:
- `node` → immediately understood, but overloaded and semantically bland.
- `nyad` → distinctive, searchable, project-specific, but requires one sentence of definition.
- It also faintly evokes **naiad** (water nymph), which gives it a “small living entity in a larger structure” feel.

If your “node” has richer semantics than merely *a vertex in a graph*, I actually prefer `nyad`. If it really is just a vertex, it risks being gratuitous vocabulary.

And `nyad` pluralizes nicely: **nyads**.

## User

Thinking to rename deltas to nyads in the vetochka, because delta is about something specific: either math or physics stuff, but nyad is just fun and goofy why not
Also need to come up with a symbol for nyads

Since there are basically three shapes, I've employed the following notation currently:
```
^0
^1[x]
^2[x, y]
```

It is somewhat cumbersome, but fits nicely to vetochka syntax in general. Want to think about the replacement for `^` and maybe better things than braces and commas

## Assistant

`nyad` actually changes what I’d optimize for: I’d stop trying to make the sigil look mathematically meaningful. The syntax can be a recognizable little *creature* in the language instead.

## Assistant

Yeah. If we’re deliberately abandoning the mathematical connotation of **delta**, I’d abandon `^` too. `^` still screams “formal operator / exponent / lattice thing”.

My strongest candidate for the nyad sigil is **`~`**. It is visually distinctive, vaguely watery/wiggly, and suitably unserious without looking ridiculous.

Then there are two particularly good directions.

**Keep the three constructors explicit:**
```text
~0
~1 x
~2 x y
```
I like this quite a lot. If `0/1/2` determine how many terms follow, brackets and commas are completely redundant. It also fits a minimal tree language better: the syntax itself tells the parser the shape.

Nested:
```text
~2 (~1 x) (~2 y z)
```
(or whatever Vetochka's normal grouping rules are).

Alternatively, if `0/1/2` are really **nothing except arity**, remove them too:
```text
~[]
~[x]
~[x y]
```
This is aesthetically nice: one construct, three shapes. Spaces instead of commas are especially appropriate for Vetochka; commas feel like conventional collection syntax imported from C-ish languages.

I currently rank them:

```text
~0
~1 x
~2 x y
```

then

```text
~[]
~[x]
~[x y]
```

then the conservative

```text
~0
~1[x]
~2[x y]
```

The interesting semantic question is whether `0`, `1`, `2` are **constructor identities** or merely a textual encoding of how many children the nyad has. If it's the latter, I think they deserve to die together with the commas. memcite

## User

Ok, tilda with number is good, but I have this grammar 
https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md

Note that it doesn't have these nodes I was talking about yet, I didn't push the changes yet. The problem is with two consecutive expressions, I lowkey think we could parse them, since we could delimit expressions, but this form falls out of the rest of the language: comma separated lists in braces or parens, statement lists in do-end blocks and labeled lists

Wanna make something coherent

## Assistant

I’ll anchor this against the actual concrete syntax, because the right nyad form should reuse one of Vetochka’s existing list/grouping conventions rather than invent a fourth one.

## Assistant

There’s one immediate collision: **`~` is already a prefix operator** in the grammar, so `~0` naturally reads as “prefix `~` applied to `0`.”  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md))

Given the existing syntax, I think the right principle is:

> **Nyad introduces a fixed-arity structural form; its children should use the existing comma-list convention rather than adjacency.**

You already use comma-separated expressions in both `[...]` and call `(...)`; semicolons belong specifically to layout/block sequencing.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md)) So I would keep:

```text
~0
~1[x]
~2[x, y]
```

rather than:

```text
~0
~1 x
~2 x y
```

The latter isn't ambiguous *in principle*, but it invents a new “arity determines how many following expressions I consume” composition rule. Nothing else in Vetochka works like that. That's the incoherence you're feeling.

I also prefer **brackets over parens** here:

```text
~1[x]
~2[x, y]
```

because `()` already means grouping/application depending on position, whereas `[]` already means structural list-ish data.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md)) A nyad is much closer to structural data than to invocation.

So the remaining problem is really **the sigil**, not the child syntax. Either:

1. **Reclaim `~` from prefix operators** and use it for nyads. This is aesthetically strongest.
2. Keep unary `~` and choose another nyad sigil.
3. Lex `~0`, `~1`, `~2` as dedicated tokens. Possible, but I dislike it: now whitespace changes category in a rather magical way:
   ```text
   ~1[x]   ;; nyad
   ~ 1[x]  ;; prefix operator
   ```

I'd seriously consider **removing `~` from generic `prefix_operator`** if you don't have an important semantics assigned to it yet. Then `~0`, `~1[...]`, `~2[...]` fits the language remarkably well: small special head + ordinary existing comma-list payload.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md))

And I wouldn't optimize away `[ ]` for arity 1. The slight verbosity buys a very useful invariant:

```text
~N[comma-list]
```

with `~0` as the sole nullary abbreviation. That's cleaner than three subtly different grammatical forms. memcite

## User

Ok, so basically change from my proposed form `^ => ~` and remove the prefix. Sane

Could we employ other existing syntactic constructions to make this into more succint form?

## Assistant

Yes. Looking at the grammar, I think there’s a cleaner possibility than merely `^ → ~`.

The obvious compression is to let **the delimiters carry the arity**:

```text
~
~[x]
~[x, y]
```

instead of

```text
~0
~1[x]
~2[x, y]
```

This reuses Vetochka’s existing `[...]` + `comma_list` vocabulary directly. The `0/1/2` are redundant because the grammar already knows the number of children.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md))

And there’s a nice deeper coherence here: **lists are already encoded using these three shapes**:

```text
[x, y] -> ^ x (^ y ^)
```

in the current spec.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md))

So after the rename, conceptually:

```text
[]      ≈ ~
[x]     ≈ ~[x, ~]
[x, y]  ≈ ~[x, ~[y, ~]]
```

That makes `~[x, y]` feel particularly natural: brackets already mean “some structural children separated by commas”, while `~` says “construct the raw nyad directly.”

Other possibilities are weaker:

```text
~()
~(x)
~(x, y)
```

Very compact, but `()` is already strongly application-shaped in Vetochka (`f(x,y)`), so this makes nyads look like constructors/functions.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md))

```text
~
~ x:
~ x: y
```

Could abuse labeled syntax, but that's plainly the wrong abstraction.

So I'd seriously consider:

```ebnf
nyad ::= "~"
       | "~" "[" expression "]"
       | "~" "[" expression "," expression "]"
```

or structurally:

```ebnf
nyad ::= "~" ("[" comma_list "]")?
```

with semantic arity restricted to ≤2.

**One concern:** `~[x,y]` visually resembles postfix `f[x,y]`, which currently means applying `f` to a proper list.  ([GitHub](https://github.com/UberDever/vetochka/blob/master/docs/spec/02_concrete_syntax.md)) But I don't think that's fatal: `~` would be a dedicated literal introducer, just as `$` is lexically special.

I actually prefer **`~ / ~[x] / ~[x,y]`** over numbered nyads now. The numbers expose an implementation classification that the concrete shape already expresses. memcite

## User

I would say that similarly to a $, ~ is a symbol that you "apply to" and you get only a subset of things to apply. In case of a dollar it's labeled syntax and do/end blocks. Space is optional here.
`$fn: [x, y, z] do end to: ... `

Similar to tilda. Then, zero case is an empty list and the rest are as you've described

## Assistant

Yes — that gives it a much stronger internal logic.

`$` and `~` become analogous **sigil heads** whose following syntax is restricted:

```text
$fn: [x, y, z] do
...
end

~[]
~[x]
~[x, y]
```

with whitespace immaterial:

```text
~ [x, y]
```

same thing as:

```text
~[x, y]
```

I like this better than `~0/~1/~2`. The arity is now expressed exactly once, structurally.

Conceptually:

```ebnf
nyad ::= "~" "[" [ expression [ "," expression ] ] "]"
```

So:
- `~[]` → nyad₀
- `~[x]` → nyad₁
- `~[x, y]` → nyad₂
- `~[x, y, z]` → syntax error

And your analogy with `$` is useful: neither really needs to mean ordinary unrestricted application. They're **special symbolic entry points into restricted syntactic families**. `$` accepts labels / `do…end`; `~` accepts a 0–2-element bracketed expression list.

That feels considerably more Vetochka-ish than encoding arity numerically. memcite