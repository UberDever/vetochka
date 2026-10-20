"""Scratchpad for task 20260926-091318. Run with standard-library Python.

This models triage head reduction. Strings are opaque payloads or symbolic
parameters. It does not define their application or piths, or model effects.
The example constructs trees directly and prints core terms. It has no parser.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class N:
    children: tuple[Term, ...] = ()


@dataclass(frozen=True)
class A:
    f: Term
    x: Term


Term = N | A | str


def n(*xs: Term) -> N:
    assert len(xs) <= 2
    return N(xs)


leaf = n()
K = n(leaf)


def const(x: Term) -> N:
    return n(leaf, x)


def spread(f: Term, g: Term) -> N:
    return n(n(f), g)


I = spread(K, K)


def triage(a: Term, b: Term, c: Term) -> N:
    return n(n(a, b), c)


def tag(t: Term, f: Term) -> N:
    return spread(spread(const(K), f), const(t))


# These readers use Rule 3 on actual children, not on pending projections.
right = triage(leaf, leaf, const(I))
get_tag = triage(leaf, leaf, const(right))
read_inner = triage(leaf, right, leaf)
untag = triage(leaf, leaf, spread(const(K), read_inner))


def step(term: Term) -> Term | None:
    if not isinstance(term, A):
        return None
    f, x = term.f, term.x
    if not isinstance(f, N):
        next_f = step(f)
        return None if next_f is None else A(next_f, x)
    if len(f.children) < 2:
        return n(*f.children, x)
    p, q = f.children
    if not isinstance(p, N):
        raise NotImplementedError("Dispatch on an opaque payload is outside this model")
    if len(p.children) == 0:
        return q
    if len(p.children) == 1:
        return A(A(p.children[0], x), A(q, x))
    if not isinstance(x, N):
        raise NotImplementedError("Runtime application and byte-string piths are undefined here")
    w, z = p.children
    if len(x.children) == 0:
        return w
    if len(x.children) == 1:
        return A(z, x.children[0])
    return A(A(q, x.children[0]), x.children[1])


def reaches(term: Term, expected: Term) -> int:
    for count in range(100):
        if term == expected:
            return count
        next_term = step(term)
        assert next_term is not None, ("stuck", expected)
        term = next_term
    raise AssertionError("step bound exceeded")


def check_laws() -> None:
    assert reaches(A(tag("t", "f"), "x"), A("f", "x")) == 5
    reaches(A(get_tag, tag("t", "f")), "t")
    reaches(A(untag, tag("t", "f")), "f")
    samples = [leaf, n(leaf), n(leaf, leaf), K, I, n(K, I), n(n(I)), n(K, n(I, K))]
    for t in samples:
        for f in samples:
            reaches(A(get_tag, tag(t, f)), t)
            reaches(A(untag, tag(t, f)), f)
            for x in samples:
                reaches(A(tag(t, f), x), A(f, x))
    # Even an application inside the annotation is not executed by forwarding.
    t = A("annotation_that_must_not_run", leaf)
    reaches(A(tag(t, "f"), "x"), A("f", "x"))


def proper_list(*values: Term) -> N:
    result = leaf
    for value in reversed(values):
        result = n(value, result)
    return result


def kv(key: Term, value: Term) -> N:
    # Candidate convention only: the pair descriptor is the bare string {:kv}.
    return tag(":kv", proper_list(key, value))


def core(term: Term) -> str:
    """Print core terms, collapsing proper-list subtrees without recognizing tags."""
    if isinstance(term, N):
        items = []
        tail: Term = term
        while isinstance(tail, N) and len(tail.children) == 2:
            items.append(tail.children[0])
            tail = tail.children[1]
        if tail == leaf:
            return "[" + ", ".join(core(item) for item in items) + "]"
        return "~[" + ", ".join(core(child) for child in term.children) + "]"
    if isinstance(term, A):
        return core(term.f) + "(" + core(term.x) + ")"
    return "{" + term + "}"


def trace(term: Term) -> list[Term]:
    result = [term]
    for _ in range(100):
        next_term = step(result[-1])
        if next_term is None:
            return result
        result.append(next_term)
    raise AssertionError("step bound exceeded")


def show_trace(label: str, term: Term, expected: Term) -> None:
    steps = trace(term)
    assert steps[-1] == expected
    print(f"\n{label}: {len(steps) - 1} steps")
    for index, state in enumerate(steps):
        print(f"{index}: {core(state)}")


def main() -> None:
    check_laws()
    print("Original 644 checks passed (bounded model checks, not a formal proof).")
    assert core(leaf) == "[]"
    assert core(proper_list("a", proper_list("b"))) == "[{a}, [{b}]]"
    assert core(n("a", "b")) == "~[{a}, {b}]"
    assert core(n("a")) == "~[{a}]"
    tag_field = kv("tag", ":i64")
    meta_field = kv("meta", leaf)
    descriptor = proper_list(tag_field, meta_field)
    an_int = tag(descriptor, "1234")
    print("\nCore integer term (no identifiers, label syntax, or tag primitive):")
    print(core(an_int))
    show_trace("getTag(an_int)", A(get_tag, an_int), descriptor)
    show_trace("untag(an_int)", A(untag, an_int), "1234")
    show_trace("getTag(tag field)", A(get_tag, tag_field), ":kv")
    show_trace("untag(tag field)", A(untag, tag_field), proper_list("tag", ":i64"))
    show_trace("getTag(meta field)", A(get_tag, meta_field), ":kv")
    show_trace("untag(meta field)", A(untag, meta_field), proper_list("meta", leaf))
    print("\nAll six example projections passed. K-v notation is a presentation convention.")


if __name__ == "__main__":
    main()
