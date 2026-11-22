"""Throwaway evaluator for the v0 subset used in force.v0."""

from dataclasses import dataclass
from pathlib import Path
import re


@dataclass(frozen=True)
class Fork:
    left: object
    right: object


@dataclass(frozen=True)
class Term:
    tree: object
    env: dict
    pending: tuple | None = None


@dataclass(frozen=True)
class Intrinsic:
    apply: object


def tree_list(items):
    result = None
    for item in reversed(items):
        result = Fork(item, result)
    return result


def elements(tree):
    while isinstance(tree, Fork):
        yield tree.left
        tree = tree.right
    if tree is not None:
        raise ValueError("expected a proper list")


TOKEN = re.compile(r"\s*(~\{[0-9a-fA-F\s]*\}|\{[^{}]*\}|~\[|~|\[|\]|,)")


def parse(source):
    tokens = iter(TOKEN.findall(source))
    if "".join(TOKEN.sub("", source).split()):
        raise ValueError("unexpected syntax")

    def read(token):
        if token == "~":
            return None
        if token.startswith("~{"):
            return bytes.fromhex(token[2:-1])
        if token.startswith("{"):
            return token[1:-1].encode("utf-8")
        if token in ("[", "~["):
            items = []
            following = next(tokens)
            while following != "]":
                items.append(read(following))
                following = next(tokens)
                if following == "]":
                    break
                if following != ",":
                    raise ValueError("expected a comma")
                following = next(tokens)
            if token == "~[":
                return Fork(*items)
            return tree_list(items)
        raise ValueError(f"unexpected token {token}")

    result = read(next(tokens))
    if next(tokens, None) is not None:
        raise ValueError("expected one tree")
    return result


def display(tree):
    if tree is None:
        return "~"
    if isinstance(tree, bytes):
        try:
            text = tree.decode("utf-8")
            if text.isprintable() and all(character not in "{}" for character in text):
                return "{" + text + "}"
        except UnicodeDecodeError:
            pass
        return "~{" + tree.hex(" ") + "}"
    try:
        return "[" + ", ".join(map(display, elements(tree))) + "]"
    except ValueError:
        return "~[" + display(tree.left) + ", " + display(tree.right) + "]"


def static_tag(tree):
    if isinstance(tree, Fork) and isinstance(tree.left, Fork):
        if tree.left.right is None and isinstance(tree.left.left, bytes):
            return tree.left.left
    return None


def lookup(term, allow_unbound=False):
    _, name = elements(term.tree)
    if name in term.env:
        return term.env[name]
    if allow_unbound:
        return term
    raise ValueError(f"unbound identifier {name!r}")


def inspect(term):
    """Follow aliases for observation, leaving applications and binders as trees."""
    while isinstance(term, Term) and static_tag(term.tree) == b":id":
        target = lookup(term, allow_unbound=True)
        if target is term:
            break
        term = target
    return term


def tag(term):
    """Read the singleton tag head through the same substitutions as triage."""
    if not isinstance(term, Term) or not isinstance(term.tree, Fork):
        return None
    head = inspect(Term(term.tree.left, term.env))
    if isinstance(head, Term) and isinstance(head.tree, Fork):
        tail = inspect(Term(head.tree.right, head.env))
        name = inspect(Term(head.tree.left, head.env))
        if isinstance(tail, Term) and tail.tree is None:
            if isinstance(name, Term) and isinstance(name.tree, bytes):
                return name.tree
    return None


def atom(term):
    if static_tag(term.tree) in (b"$", b":binder"):
        raise ValueError("expected an ANF atom")
    if static_tag(term.tree) == b":id":
        return lookup(term, allow_unbound=True)
    return term


def enter(term):
    """Enter a body. A returned tree goes directly to its caller."""
    if isinstance(term, Intrinsic):
        return term
    if static_tag(term.tree) == b":binder":
        return run_binder(term)
    if static_tag(term.tree) == b":id":
        return lookup(term)
    return term


def run_binder(term):
    _, bindings, body = elements(term.tree)
    (binding,) = elements(bindings)
    value = step(Term(binding.right, term.env))
    return enter(Term(body, term.env | {binding.left: value}))


def unset_identifier(name):
    # An empty context keeps an unset pattern name from finding an outer binding.
    return Term(tree_list([tree_list([b":id"]), name]), {})


def triage_tail_env(subject):
    pending = subject.pending
    if pending is None:
        try:
            if tag(subject) != b":lambda":
                return subject.env
            _, declared, _ = elements(subject.tree)
            pending = tuple(elements(declared))
        except ValueError:
            # Malformed control trees are still available for structural inspection.
            return subject.env
        if not all(isinstance(name, bytes) for name in pending):
            return subject.env

    # Keep the unfilled declarations in scope when triage extracts the body.
    return subject.env | {name: unset_identifier(name) for name in pending}


def apply(callee, operand):
    if isinstance(callee, Intrinsic):
        return Term(callee.apply(operand), {})
    match tag(callee):
        case b":lambda":
            _, declared, body = elements(callee.tree)
            name, *remaining = callee.pending or tuple(elements(declared))
            env = callee.env | {name: operand}
            if remaining:
                return Term(callee.tree, env, tuple(remaining))
            return enter(Term(body, env))
        case b":triage":
            _, names, leaf, fork, fruit = elements(callee.tree)
            lhs, rhs = elements(names)
            subject = inspect(operand)
            if not isinstance(subject, Term):
                raise ValueError("this spike has no triage projection for an intrinsic")
            if subject.tree is None:
                body = leaf
                left, right = unset_identifier(lhs), unset_identifier(rhs)
            elif isinstance(subject.tree, Fork):
                body = fork
                left = Term(subject.tree.left, subject.env)
                right = Term(subject.tree.right, triage_tail_env(subject))
            else:
                body = fruit
                left, right = subject, unset_identifier(rhs)
            return enter(Term(body, callee.env | {lhs: left, rhs: right}))
        case _:
            raise ValueError("expected callable")


def step(term):
    """Resolve a binder RHS and perform its selected operation once."""
    term = inspect(term)
    match tag(term):
        case b"$":
            _, callee, operand = elements(term.tree)
            return apply(inspect(atom(Term(callee, term.env))), atom(Term(operand, term.env)))
        case b":binder":
            return run_binder(term)
        case b":id":
            return lookup(term)
        case _:
            return term


def fruit_equal(argument):
    if not isinstance(argument, Term):
        raise ValueError("expected a list of two fruits")
    operands = [inspect(Term(item, argument.env)) for item in elements(argument.tree)]
    first, second = operands
    if all(isinstance(value, Term) and isinstance(value.tree, bytes) for value in operands):
        if first.tree == second.tree:
            return b"true"
    return None


def environment(intrinsics):
    return {name.encode(): Intrinsic(action) for name, action in (intrinsics or {}).items()}


def run(source, intrinsics=None):
    return display(enter(Term(parse(source), environment(intrinsics))).tree)


DRIVER_CALL = parse("""[
  [{:binder}],
  [~[{result}, [[{$}], [[{:id}], {force}], [[{:id}], {input}]]]],
  [[{:id}], {result}]
]""")


def force(source, intrinsics=None):
    env = environment(intrinsics)
    library = parse(Path(__file__).with_name("force.v0").read_text())
    driver = enter(Term(library, {b"fruit-equal": Intrinsic(fruit_equal)}))
    env[b"input"] = Term(parse(source), env.copy())
    env[b"force"] = driver
    return display(enter(Term(DRIVER_CALL, env)).tree)


if __name__ == "__main__":
    import sys

    print(force(sys.stdin.read()))
