# Introduction

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
