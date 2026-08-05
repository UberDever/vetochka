# Vetochka foundation

## Motivation

Yes, this is an another language. My goals for creation of the new language are simple:
I want to make **my** language, as I understand the PLD currently and I want to improve my PLD skills.

That said, this language is not entirely superfluos and artificial. My very first
desire, when it comes to programming languages, is the need to greatly reduce
complexity. Like, at all costs. As an engineer I understand that this approach will complicate
execution of other requirements, mainly performance. 
But the wish for simplification is very deep, it is almost
innate or rather immanent to myself. So, I can put aside my "engineer" hat and
put back a researcher one, as I explore new possibilities and quirky problems.

Therefore, this language is unordinary even for functional language. It is based on relatively
unknown calculus called `tree-calculus`. This calculus interested me for couple of reasons. First,
it is niche and relatively unknown. Second, it proposes very bald takes about Turing-Church thesis
and Lambda calculus. As you can tell this points are the nerdy excitement and the counterculture
as is. But there is another, third point: this calculus has "reflection" at its core. In fact,
one of the three reduction rules of the **entire** calculus is about the conditional choice, based
upon the provided argument. Pure reflection at its core. And of course it means that this calculus
and my language is very unoptimal and slow, because there is **truly** no difference between
program and data and, as such, no related optimizations.

Okay, what about applications? To design a language is to be able to apply it somewhere, or else
it stays a mathematical model in Plato's realm of forms. Well, I have rather strange and somewhat unhinged
application for my language -- "Mighty preprocessor". Preprocessor for one and only -- for `C`.

Does this make any sense? If you think about it, why not? `C` is an old language, its
features are error-prone and unreliable, some modern features are entirely absent.
If we can add a buffed macro language we can at least be able to extend the granny with new
higher-level features and custom DSLs. Well, safety is not a goal here, since my preprocessor is
an extension of a language -- it can't (and hopefully won't) change the underlying semantics.
That said, an ability to add a dynamic dispatch or proper sum types without code duplication
seems appealing. Also, the functional nature of my language is the ideal (in my opinion) bet for the perprocessor --
the language without side-effects and (hopefully) total.

Now, whether `C` needs an another "higher" language (or extension) is a separate matter.
I think that a good approach for current production systems would be to migrate from `C` to
`C fixed` or something -- the language that **almost entirely** supports `C` as a subset, but
also fixes some compiler and runtime issues. `C++`? Sadly, it's its own ball of mud. They even currently
try to create such and extension to fix some `C++` issues, that are present from `C` times!
That said, we need some sort of `Typescript` for `C` and when (or if) it will be created,
many system programming concerns would become simpler. For the meantime, as `C` is still
around (and would be), I'm tinkering around with the concept of macro language.

## Tree calculus foundation

### Spec

Tree calculus is Vetochka foundation.

Terms are built from:

- `^` — the single primitive value;
- binary application.

In these rules, `$` marks binary application.

The three observable shapes are:

```text
leaf: ^
stem: ^ x
fork: ^ x y
```

Reduction rules:

```text
0a. ^ $ x                  -> ^ x
0b. ^ x $ y                -> ^ x y
1.  ^ ^ x $ y              -> x
2.  ^ (^ x) y $ z          -> (x z) (y z)
3a. ^ (^ w x) y $ ^        -> w
3b. ^ (^ w x) y $ (^ u)    -> x u
3c. ^ (^ w x) y $ (^ u v)  -> y u v
```

This calculus has no literals, names, cells, opcodes, machine state, effects,
environments, modules, or execution policy beyond its reduction relation.

### Rationale

Tree calculus gives Vetochka reflective foundation: rule 3 branches on the shape of
a value. Higher layers may add practical representation and execution machinery,
but they must not change these rules for the delta/triage case.

### Open

None for the current v0 spec.

### Notes

- `$` is explanatory notation for this section only.
- Surface/calculus text is application-first: `^ ^ ^` means `((^ ^) ^)`. Leaf,
  stem, and fork are reduction/storage shapes reached through Rules 0a/0b, not
  separate source constructors.
- Proper lists are a higher-layer convention encoded with right-nested forks:
  `[x, y, z] == ^ x (^ y (^ z ^))`, `[] == ^`.
- Confluence/order-independence is relied on only in the pure Layer 0 setting.
  Once effects or host operations exist, Layer 5 must define observable order.
- Older notes sometimes describe literals or opcodes as leaf/stem/fork-shaped.
  That is a higher-layer compatibility claim, not Layer 0 vocabulary.
