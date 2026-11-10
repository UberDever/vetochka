"""Exploratory self-interpreter for pure triage. Run with standard-library Python.

This is not a runtime change or an approved quotation format. Python constructs
the interpreter term, quotes inputs, and reads outputs. The harness executes
only native triage reductions, including demanded function-shape exposure.
"""
from dataclasses import dataclass
from pathlib import Path

from tagging_scratchpad import (
    A, I, K, N, Term, const, core, leaf, n, spread, step, triage,
)


@dataclass(frozen=True)
class Lambda:
    variable: str
    body: object


def call(f, *args):
    for arg in args:
        f = A(f, arg)
    return f


def lam(*variables_and_body):
    *variables, body = variables_and_body
    for variable in reversed(variables):
        body = Lambda(variable, body)
    return body


def occurs(variable: str, term: Term) -> bool:
    if isinstance(term, str):
        return term == variable
    if isinstance(term, A):
        return occurs(variable, term.f) or occurs(variable, term.x)
    return any(occurs(variable, child) for child in term.children)


def abstract(variable: str, term: Term) -> Term:
    """Bracket abstraction for Vetochka's S order, not Jay's reversed order."""
    if not occurs(variable, term):
        return const(term)
    if term == variable:
        return I
    if isinstance(term, N):
        term = call(leaf, *term.children)
    assert isinstance(term, A)
    if term.x == variable and not occurs(variable, term.f):
        return term.f
    return spread(abstract(variable, term.f), abstract(variable, term.x))


def compile_term(expr) -> Term:
    if isinstance(expr, Lambda):
        return abstract(expr.variable, compile_term(expr.body))
    if isinstance(expr, A):
        return A(compile_term(expr.f), compile_term(expr.x))
    return expr


# Scott quotation: a term selects one of four supplied handlers.
# Constructors are ordinary combinators. Application of Q_APP constructs code.
Q_LEAF = compile_term(lam("l", "s", "f", "a", "l"))
Q_STEM = compile_term(lam("p", "l", "s", "f", "a", call("s", "p")))
Q_FORK = compile_term(lam("p", "q", "l", "s", "f", "a", call("f", "p", "q")))
Q_APP = compile_term(lam("p", "q", "l", "s", "f", "a", call("a", "p", "q")))


def quote(term: Term) -> Term:
    """Parser-side encoding. Does not execute applications in the input."""
    if isinstance(term, A):
        return call(Q_APP, quote(term.f), quote(term.x))
    assert isinstance(term, N), "This prototype accepts only pure triage terms"
    if not term.children:
        return Q_LEAF
    constructor = Q_STEM if len(term.children) == 1 else Q_FORK
    return call(constructor, *(quote(child) for child in term.children))


def again(term):
    return call("self", "self", term)


# Leaf/stem/fork input is already a result. For an application, expose only
# the function root, then its left child if it is a fork. Only rule 3 exposes x.
# The fourth handler is unreachable after again() returns a valid result.
# Malformed encodings are outside this prototype's contract.
worker = lam(
    "self", "t",
    call(
        "t",
        "t",
        lam("p", "t"),
        lam("p", "q", "t"),
        lam(
            "f", "x",
            call(
                again("f"),
                call(Q_STEM, "x"),                           # 0a
                lam("p", call(Q_FORK, "p", "x")),             # 0b
                lam(
                    "p", "q",
                    call(
                        again("p"),
                        again("q"),                         # 1
                        lam(
                            "u",
                            again(call(Q_APP,               # 2
                                call(Q_APP, "u", "x"),
                                call(Q_APP, "q", "x"))),
                        ),
                        lam(
                            "w", "v",
                            call(
                                again("x"),
                                again("w"),                 # 3a
                                lam("u", again(call(Q_APP, "v", "u"))),
                                lam("u", "z", again(call(Q_APP,
                                    call(Q_APP, "q", "u"), "z"))),
                                lam("u", "z", "t"),
                            ),
                        ),
                        lam("u", "z", "t"),
                    ),
                ),
                lam("u", "z", "t"),
            ),
        ),
    ),
)
WORKER = compile_term(worker)
EXEC = call(WORKER, WORKER)


class Fuel:
    def __init__(self, remaining: int = 200_000):
        self.remaining = remaining
        self.used = 0

    def head(self, term: Term) -> Term:
        while True:
            next_term = native_step(term)
            if next_term is None:
                return term
            if self.remaining == 0:
                raise RuntimeError("Native reduction step bound exceeded")
            self.remaining -= 1
            self.used += 1
            term = next_term


def native_step(term: Term) -> Term | None:
    """Expose the function's dispatch shape; never force its operand.

    The older scratchpad's step() requires the fork's left child to be a
    constructor already. Bracket abstraction can compute that child.
    """
    if not isinstance(term, A):
        return None
    f = term.f
    if not isinstance(f, N):
        next_f = native_step(f)
        return None if next_f is None else A(next_f, term.x)
    if len(f.children) == 2 and isinstance(f.children[0], A):
        p, q = f.children
        next_p = native_step(p)
        if next_p is None:
            return None
        return A(n(next_p, q), term.x)
    return step(term)


def read_code(term: Term, fuel: Fuel) -> Term:
    """Observe the Scott encoding; do not execute the represented program."""
    observed = fuel.head(call(term, "leaf", "stem", "fork", "application"))
    args = []
    while isinstance(observed, A):
        args.append(observed.x)
        observed = observed.f
    args.reverse()
    assert isinstance(observed, str), "A quoted term must select a handler"
    arity = {"leaf": 0, "stem": 1, "fork": 2, "application": 2}[observed]
    assert len(args) == arity, (observed, len(args))
    children = [read_code(arg, fuel) for arg in args]
    return A(*children) if observed == "application" else n(*children)


def interpret(term: Term) -> Term:
    return read_code(call(EXEC, quote(term)), Fuel())


def check_closed(term: Term) -> None:
    if isinstance(term, N):
        for child in term.children:
            check_closed(child)
    else:
        assert isinstance(term, A), "The interpreter contains a free name"
        check_closed(term.f)
        check_closed(term.x)


def check_exec() -> None:
    check_closed(EXEC)
    duplicate = spread(I, I)
    omega = A(duplicate, duplicate)
    cases = [
        ("leaf value", leaf, leaf),
        ("0a", A(leaf, K), n(K)),
        ("0b", A(n(K), I), n(K, I)),
        ("1 discards divergence", A(n(leaf, K), omega), K),
        ("2 preserves both raw copies", A(spread(leaf, leaf), omega),
         n(omega, A(leaf, omega))),
        ("3a", A(triage(K, leaf, leaf), leaf), K),
        ("3b", A(triage(K, leaf, leaf), n(I)), n(I)),
        ("3c", A(triage(K, leaf, leaf), n(K, I)), n(K, I)),
        ("3 observes the operand root", A(triage(K, leaf, leaf), A(leaf, omega)),
         n(omega)),
        ("constructor preserves divergent child", n(omega), n(omega)),
        ("dispatch exposes the function's left child",
         A(n(A(n(leaf, leaf), omega), K), omega), K),
    ]
    for name, term, expected in cases:
        assert read_code(quote(term), Fuel()) == term, f"quotation: {name}"
        assert interpret(term) == expected, name
        print(f"PASS {name}")
    try:
        read_code(call(EXEC, quote(omega)), Fuel(1000))
    except RuntimeError as error:
        assert str(error) == "Native reduction step bound exceeded"
    else:
        raise AssertionError("Executing omega unexpectedly terminated")
    print("PASS executing omega exhausts 1000 steps (bounded evidence)")


if __name__ == "__main__":
    check_exec()
    path = Path(__file__).with_name("exec-core.vf")
    path.write_text(core(EXEC) + "\n")
    print(f"Wrote {path.name}: pure core interpreter term")
