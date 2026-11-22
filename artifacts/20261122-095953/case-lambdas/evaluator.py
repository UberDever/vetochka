"""Throwaway evaluator for case-lambdas and binders, written fresh for (22.11.26, 26).

Rules implemented, with their log sources:

- Trees: `None` is a leaf, `bytes` is a fruit, `Fork` is a fork.
- Identifiers are `[[{:id}], {name}, ~[{:private}, <identity>]]`. Lookup matches the identity
  bytes, never the spelling (21.11.26, 0). Unforgeability is policy only (22.11.26, 0).
- Lambda: `[[{:lambda}], <private-state>, <header>, <case>, ...]` (22.11.26, 15), (22.11.26, 16).
  The header is a list of positions; each position is a list of one or two identifiers
  (22.11.26, 18). A case is a pair `~[<shape-row>, <expr>]` with shapes `{:leaf}`, `{:fork}`,
  `{:fruit}`, or the wildcard `{:all}` (22.11.26, 20).
- Each argument binds its position at once (22.11.26, 15). A one-name position binds the whole
  operand. A two-name position binds by the operand's shape: a fork binds both children, a fruit
  binds the first name to the whole fruit, a leaf binds neither (20.11.26, 5). An unset name
  shadows outer bindings and stays identifier data.
- The case is selected only when all arguments are bound (22.11.26, 14), first match in written
  order (22.11.26, 17). No match, a malformed row, and `{:fork}` at a one-name position are
  errors (22.11.26, 16), (22.11.26, 18).
- A partial application is a CLOSURE whose tree replaces the private state with the count of
  bound arguments (22.11.26, 19) and shares the cases (22.11.26, 7).
- Binder: `[[{:binder}], [~[<id>, <rhs>], ...], <expr>]`. Clauses bind in order (22.11.26, 22);
  each right-hand side is forced one step (23.10.26, 3), (20.11.26, 4).
- Shape inspection never forces (20.11.26, 3).

- Identifiers are a rewriting optimization (22.11.26, 28). An unbound identifier is a hole; a
  bound identifier is the tree it rewrites to. When an occurrence rewrites to identifier data,
  that identifier sits where the occurrence was and is rewritten in the occurrence's environment.
  Execution, shape inspection, and display all use this one rule (`rewrite`).
- A lambda's header is rewritten at its first application and then wrapped in an empty
  environment (22.11.26, 29), so a partial application's own bindings cannot rewrite its
  declarations. A tree may hold such a wrapped term as a node, as cells hold CLOSURE refs.

- An authored lambda carries `~` in its private-state slot (22.11.26, 31).
- Every position of a lambda or binder, including list tails, cases, and clauses, is read
  through `rewrite`, so a bound identifier anywhere is its tree (22.11.26, 28).
"""

from dataclasses import dataclass
import re


class VError(Exception):
    """An "it is an error" situation; the experiment stops with a Python exception."""


@dataclass(frozen=True)
class Fork:
    left: object
    right: object


@dataclass(frozen=True, eq=False)
class Term:
    """A tree with the environment it is read in (a CLOSURE when it is a control value)."""

    tree: object
    env: dict
    unset: bool = False  # an unset pattern name, held as identifier data


@dataclass(frozen=True, eq=False)
class Intrinsic:
    name: str
    apply: object


LIMIT = 10_000
SHAPES = (b":leaf", b":fork", b":fruit", b":all")
WRAP_HEADER = True  # ablation switch: False keeps the raw header in a partial application


# ---------------------------------------------------------------- trees


def tree_list(items):
    result = None
    for item in reversed(items):
        result = Fork(item, result)
    return result


def elements(tree):
    items = []
    while isinstance(tree, Fork):
        items.append(tree.left)
        tree = tree.right
    if tree is not None:
        raise VError("expected a proper list")
    return items


def is_list(tree):
    try:
        elements(tree)
        return True
    except VError:
        return False


def static_kind(tree):
    """The kind fruit of a singleton tag head `[[{kind}], ...]`, if any."""
    if isinstance(tree, Fork) and isinstance(tree.left, Fork):
        head = tree.left
        if isinstance(head.left, bytes) and head.right is None:
            return head.left
    return None


def is_identifier(tree):
    if static_kind(tree) != b":id" or not is_list(tree):
        return False
    items = elements(tree)
    return (
        len(items) == 3
        and isinstance(items[1], bytes)
        and isinstance(items[2], Fork)
        and items[2].left == b":private"
        and isinstance(items[2].right, bytes)
    )


def identity(tree):
    return elements(tree)[2].right


def make_identifier(name, key):
    return tree_list([tree_list([b":id"]), name, Fork(b":private", key)])


# ---------------------------------------------------------------- parsing and display

TOKEN = re.compile(r"\s*(~\{[0-9a-fA-F\s]*\}|\{[^{}]*\}|~\[|~|\[|\]|,)")


def parse(source):
    tokens = TOKEN.findall(source)
    if "".join(TOKEN.sub("", source).split()):
        raise ValueError("unexpected syntax")
    position = 0

    def take():
        nonlocal position
        if position >= len(tokens):
            raise ValueError("unexpected end")
        position += 1
        return tokens[position - 1]

    def read(token):
        if token == "~":
            return None
        if token.startswith("~{"):
            return bytes.fromhex(token[2:-1])
        if token.startswith("{"):
            return token[1:-1].encode("utf-8")
        if token in ("[", "~["):
            items = []
            following = take()
            while following != "]":
                items.append(read(following))
                following = take()
                if following == ",":
                    following = take()
                elif following != "]":
                    raise ValueError("expected a comma")
            if token == "~[":
                if len(items) != 2:
                    raise ValueError("a pair has exactly two elements")
                return Fork(*items)
            return tree_list(items)
        raise ValueError(f"unexpected token {token}")

    result = read(take())
    if position != len(tokens):
        raise ValueError("expected one tree")
    return result


def display(tree):
    if tree is None:
        return "~"
    if isinstance(tree, bytes):
        try:
            text = tree.decode("utf-8")
            if text and text.isprintable() and not set(text) & set("{},[]~ "):
                return "{" + text + "}"
        except UnicodeDecodeError:
            pass
        return "~{" + tree.hex(" ") + "}"
    if is_list(tree):
        return "[" + ", ".join(map(display, elements(tree))) + "]"
    return "~[" + display(tree.left) + ", " + display(tree.right) + "]"


# ---------------------------------------------------------------- context


def at(node, env):
    """A tree node read in `env`; a node that is already a term keeps its own environment."""
    return node if isinstance(node, Term) else Term(node, env)


def rewrite(value):
    """Rewrite bound identifiers to their trees; stop at a hole or a non-identifier. Never forces.

    Following an occurrence bound in its own environment moves to its value. If that value is
    identifier data unbound in its own environment, it stands where the occurrence stood, so it
    is rewritten in the occurrence's environment (22.11.26, 28).
    """
    place = None
    for _ in range(LIMIT):
        if not (isinstance(value, Term) and not value.unset and is_identifier(value.tree)):
            return value
        key = identity(value.tree)
        following = value.env.get(key)
        if following is not None and following is not value:
            place = value.env
        elif place is not None and place.get(key) not in (None, value):
            following = place[key]
        else:
            return value
        value = following
    raise VError("identifier cycle")


view = rewrite


def shape(value):
    value = view(value)
    if isinstance(value, Intrinsic):
        raise VError("an intrinsic has no shape in this experiment")
    if value.tree is None:
        return b":leaf"
    if isinstance(value.tree, Fork):
        return b":fork"
    return b":fruit"


def children(value):
    value = view(value)
    return at(value.tree.left, value.env), at(value.tree.right, value.env)


def unset(name):
    return Term(name, {}, unset=True)


def declaration(term):
    """Read a declaration position through context (C2)."""
    declared = view(term)
    if not (isinstance(declared, Term) and is_identifier(declared.tree)):
        raise VError("a declaration must be an identifier")
    return declared.tree


# ---------------------------------------------------------------- execution


def spine(term):
    """Read a proper list through context: rewrite every cell, return the element terms.

    Under (22.11.26, 28) a bound identifier anywhere in a list, including its tail, is the tree it
    rewrites to. Also returns the term for the rest after each element, for sharing.
    """
    cells = []
    current = view(term)
    for _ in range(LIMIT):
        if isinstance(current, Intrinsic) or current.unset:
            raise VError("expected a proper list")
        if current.tree is None:
            return cells
        if not isinstance(current.tree, Fork):
            raise VError("expected a proper list")
        rest = view(at(current.tree.right, current.env))
        cells.append((at(current.tree.left, current.env), rest))
        current = rest
    raise VError("list too long")


def items(term):
    return [element for element, _ in spine(term)]


def kind(value):
    """The kind fruit of a singleton tag head, read through context."""
    value = view(value)
    if not isinstance(value, Term):
        return None
    if value.unset or is_identifier(value.tree):
        return b":id"
    if not isinstance(value.tree, Fork):
        return None
    head = view(at(value.tree.left, value.env))
    if not (isinstance(head, Term) and isinstance(head.tree, Fork)):
        return None
    name = view(at(head.tree.left, head.env))
    rest = view(at(head.tree.right, head.env))
    if isinstance(name, Term) and isinstance(name.tree, bytes) and isinstance(rest, Term):
        if rest.tree is None:
            return name.tree
    return None


def operand(term):
    """An ANF atom: identifiers are rewritten, nothing is executed."""
    if static_kind(term.tree) in (b"$", b":binder"):
        raise VError("expected an ANF atom")
    if is_identifier(term.tree):
        return rewrite(term)
    return term


def enter(term):
    """Enter a body expression. A returned tree goes directly to the caller."""
    if is_identifier(term.tree) and not term.unset:
        if identity(term.tree) not in term.env:
            raise VError("unbound identifier")
        return rewrite(term)
    if kind(term) == b":binder":
        return run_binder(term)
    return term


def step(term):
    """Force a binder right-hand side: perform its operation once."""
    value = rewrite(term)
    if isinstance(value, Intrinsic):
        return value
    match kind(value):
        case b"$":
            parts = items(value)
            if len(parts) != 3:
                raise VError("an application has one callee and one operand")
            return apply(view(operand(parts[1])), operand(parts[2]))
        case b":binder":
            return run_binder(value)
        case b":id":
            raise VError("cannot run an unbound identifier")
    return value


def apply(callee, argument):
    if isinstance(callee, Intrinsic):
        return Term(callee.apply(argument), {})
    if kind(callee) == b":lambda":
        return apply_lambda(callee, argument)
    raise VError("expected a callable")


def progress(state):
    state = view(state)
    tree = state.tree
    if tree is None:
        return 0
    if isinstance(tree, Fork) and tree.left == b":private" and isinstance(tree.right, bytes):
        return int.from_bytes(tree.right, "big")
    raise VError("malformed lambda private state")


def private_count(count):
    return Fork(b":private", count.to_bytes(1, "big"))


def read_header(term):
    """A lambda's header, read through context (22.11.26, 29)."""
    positions = []
    for position in items(term):
        positions.append(tuple(declaration(name) for name in items(position)))
    return positions


def check_header(positions):
    if not positions:
        raise VError("a lambda needs at least one parameter")
    keys = []
    for names in positions:
        if len(names) not in (1, 2):
            raise VError("a position binds one or two names")
        keys.extend(identity(name) for name in names)
    if len(keys) != len(set(keys)):
        raise VError("a header declares an identity twice")


def bind(names, argument):
    if len(names) == 1:
        return {identity(names[0]): argument}
    left_name, right_name = names
    left_key, right_key = identity(left_name), identity(right_name)
    match shape(argument):
        case b":fork":
            left, right = children(argument)
            return {left_key: left, right_key: right}
        case b":fruit":
            return {left_key: argument, right_key: unset(right_name)}
        case _:
            return {left_key: unset(left_name), right_key: unset(right_name)}


def parameters(env, positions):
    """The bindings this lambda has made so far, recovered from its header identities."""
    return {
        identity(name): env[identity(name)]
        for names in positions
        for name in names
        if identity(name) in env
    }


def node(term, env):
    """A tree node for `term` inside a tree read in `env`: raw when the context agrees."""
    return term.tree if term.env is env and not term.unset else term


def apply_lambda(callee, argument):
    cells = spine(callee)
    if len(cells) < 4:
        raise VError("a lambda needs a header and at least one case")
    (tag, _), (state, _), (header, cases_rest), *case_cells = cells
    count = progress(state)
    positions = read_header(header)
    check_header(positions)
    if count >= len(positions):
        raise VError("malformed lambda progress")
    env = callee.env | bind(positions[count], argument)
    count += 1
    if count < len(positions):
        if WRAP_HEADER:
            stored = Term(tree_list([tree_list(list(names)) for names in positions]), {})
        else:
            stored = node(header, callee.env)
        tree = Fork(
            node(tag, callee.env),
            Fork(private_count(count), Fork(stored, node(cases_rest, callee.env))),
        )
        return Term(tree, env)
    return select(positions, [case for case, _ in case_cells], env)


def bound(env, name):
    key = identity(name)
    if key not in env:
        raise VError("a header parameter is not bound")
    return env[key]


def position_shape(env, names):
    if len(names) == 1:
        return shape(bound(env, names[0]))
    left = bound(env, names[0])
    right = bound(env, names[1])
    if getattr(left, "unset", False) and getattr(right, "unset", False):
        return b":leaf"
    if getattr(right, "unset", False):
        return b":fruit"
    return b":fork"


def select(positions, cases, env):
    params = parameters(env, positions)
    rows = []
    for case in cases:
        case = view(case)
        if not (isinstance(case, Term) and isinstance(case.tree, Fork)):
            raise VError("a case is a pair of a shape row and an expression")
        shapes = []
        for item in items(at(case.tree.left, case.env)):
            symbol = view(item)
            if not (isinstance(symbol, Term) and symbol.tree in SHAPES):
                raise VError("a shape row holds shape symbols")
            shapes.append(symbol.tree)
        if len(shapes) != len(positions):
            raise VError("a shape row must cover every parameter")
        for expected, names in zip(shapes, positions):
            if expected == b":fork" and len(names) == 1:
                raise VError("{:fork} needs a two-name position")
        body = at(case.tree.right, case.env)
        # A case read from another context still sees this lambda's parameters.
        rows.append((shapes, Term(body.tree, body.env | params, body.unset)))

    actual = {}

    def actual_shape(index):
        # Computed only when a row inspects the position, so an all-wildcard lambda
        # accepts operands that have no shape (intrinsics).
        if index not in actual:
            actual[index] = position_shape(env, positions[index])
        return actual[index]

    for shapes, body in rows:
        if all(
            expected == b":all" or expected == actual_shape(index)
            for index, expected in enumerate(shapes)
        ):
            return enter(body)
    raise VError("no case matches")


def run_binder(term):
    parts = items(term)
    if len(parts) != 3:
        raise VError("a binder has clauses and one body")
    _, clauses, body = parts
    clauses = items(clauses)
    if not clauses:
        raise VError("a binder needs at least one clause")
    made = {}
    for clause in clauses:
        clause = view(clause)
        if not (isinstance(clause, Term) and isinstance(clause.tree, Fork)):
            raise VError("a clause is a pair of a declaration and an expression")
        # Clauses read from another context still see the bindings made so far.
        scope = clause.env | made
        key = identity(declaration(Term(clause.tree.left, scope)))
        made[key] = step(Term(clause.tree.right, scope))
    body_scope = body.env | made
    return enter(Term(body.tree, body_scope, body.unset))


# ---------------------------------------------------------------- results


def materialize(value, depth=0):
    """Observe a result through its substitutions, for display only."""
    if depth > 500:
        raise VError("result too deep to display")
    if isinstance(value, Intrinsic):
        return ("<intrinsic " + value.name + ">").encode()
    value = rewrite(value)
    if isinstance(value, Intrinsic):
        return ("<intrinsic " + value.name + ">").encode()
    tree = value.tree
    if value.unset or is_identifier(tree) or not isinstance(tree, Fork):
        return tree
    return Fork(
        materialize(at(tree.left, value.env), depth + 1),
        materialize(at(tree.right, value.env), depth + 1),
    )


# ---------------------------------------------------------------- host


UNIQUE_KEY = b"\xf0"


def global_env():
    counter = iter(range(0x8000, 0x10000))

    def unique(argument):
        name = view(argument)
        if not (isinstance(name, Term) and isinstance(name.tree, bytes)):
            raise VError("unique expects a name fruit")
        return make_identifier(name.tree, next(counter).to_bytes(2, "big"))

    return {UNIQUE_KEY: Intrinsic("unique", unique)}


def run(tree):
    """Enter a whole program and return the raw result value."""
    return enter(Term(tree, global_env()))


def run_display(tree):
    return display(materialize(run(tree)))
