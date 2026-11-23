"""Reference machine for v0. Throwaway; the real one is written by hand.

Substitution is literal rewriting: applying a lambda or running a binder copies the tree with the
bound identities replaced. There is no environment, so the machine is C, S, K: a control tree,
a store, and a stack of continuation frames. It behaves as in-place rewriting by construction
(23.11.26, 0), which is what deferred implementations are tested against.

Trees: `None` is a leaf, `bytes` a fruit, `Fork` a fork.

Rewriting (`subst`)
- An identifier `[[{:id}], {name}, ~[{:private}, <identity>]]` whose identity is substituted is
  replaced by the value; the value is inserted as it is, not rewritten further by the same step.
- General shadowing (23.11.26, 0): a lambda's declarations are the identifiers written directly
  in its header positions; a binder clause's declaration is the identifier written as its left
  side. Rewriting does not replace those, and stops replacing a declared identity inside the
  lambda's cases, or in a binder's later clauses and body.
- Anything else written in a declaration slot, such as an occurrence standing for a whole header
  or clause list, is rewritten like any other tree.
- A hole (an identifier nothing replaces) stays; it is data under shape inspection and an error
  when run.

Steps
- `enter t`: a binder runs, however it was reached (23.11.26, 0); anything else is returned.
- `run b`: force the first clause's right-hand side with a `bind` frame on the stack.
- `force t`: an application `[[{$}], callee, operand]` applies; a binder runs; a hole is an error;
  anything else is a value. Operands are passed as trees, never forced.
- `return v`: pop a `bind` frame, rewrite the remaining clauses and the body with the declaration
  bound to `v`, and continue; with no frame left, `v` is the result.
- Applying a lambda binds the next parameter position by the operand's shape and rewrites the
  cases with it (22.11.26, 3). Until the last position, the result is the lambda with rewritten
  cases and private state `~[{:private}, <count, shapes>]` (implementation-specific bytes). At
  the last position the first case whose shape row matches is selected (22.11.26, 4) and entered,
  which is a tail call: no frame is pushed.
- An intrinsic is a tree `[[{:intrinsic}], {name}]`; it takes its arguments as a list
  (12.11.26, 11). The store maps addresses `[[{:address}], ~[{:private}, n]]` to trees
  (22.11.26, 18).

Errors raise `MachineError` ("it is an error"). Host recursion happens only over tree depth in
`subst`, never over program steps.
"""

from dataclasses import dataclass
import re
import sys

sys.setrecursionlimit(50_000)


class MachineError(Exception):
    pass


@dataclass(frozen=True)
class Fork:
    left: object
    right: object


SHAPES = (b":leaf", b":fork", b":fruit", b":all")
SHAPE_CODE = {b":leaf": 0, b":fork": 1, b":fruit": 2}
CODE_SHAPE = {code: shape for shape, code in SHAPE_CODE.items()}


# ---------------------------------------------------------------- trees


def lst(items):
    result = None
    for item in reversed(items):
        result = Fork(item, result)
    return result


def kind(tree):
    """The kind fruit of a singleton tag head `[[{kind}], ...]`, if any."""
    if isinstance(tree, Fork) and isinstance(tree.left, Fork):
        head = tree.left
        if isinstance(head.left, bytes) and head.right is None:
            return head.left
    return None


def is_identifier(tree):
    if kind(tree) != b":id":
        return False
    items, tail = spine(tree, stop_at_identifiers=False)
    return (
        tail is None
        and len(items) == 3
        and isinstance(items[1], bytes)
        and isinstance(items[2], Fork)
        and items[2].left == b":private"
        and isinstance(items[2].right, bytes)
    )


def identity(tree):
    return tree.right.right.left.right


def make_identifier(name, key):
    return lst([lst([b":id"]), name, Fork(b":private", key)])


def spine(tree, stop_at_identifiers=True):
    """List cells as written: (elements, tail). The tail is `None` for a proper list; a tail
    that is an identifier is an occurrence still to be rewritten, not more list cells."""
    items = []
    while isinstance(tree, Fork) and not (stop_at_identifiers and is_identifier(tree)):
        items.append(tree.left)
        tree = tree.right
    return items, tree


def elements(tree, what):
    items, tail = spine(tree)
    if tail is not None:
        raise MachineError(f"{what} must be a proper list")
    return items


def shape(tree):
    if tree is None:
        return b":leaf"
    if isinstance(tree, Fork):
        return b":fork"
    return b":fruit"


# ---------------------------------------------------------------- rewriting


def subst(tree, sigma):
    """Rewrite `tree` with `sigma` (identity bytes -> tree), honouring general shadowing."""
    if not sigma or not isinstance(tree, Fork):
        return tree
    if is_identifier(tree):
        return sigma.get(identity(tree), tree)
    if kind(tree) == b":lambda":
        return subst_lambda(tree, sigma)
    if kind(tree) == b":binder":
        return subst_binder(tree, sigma)
    left, right = subst(tree.left, sigma), subst(tree.right, sigma)
    if left is tree.left and right is tree.right:
        return tree
    return Fork(left, right)


def rebuild(items, tail):
    result = tail
    for item in reversed(items):
        result = Fork(item, result)
    return result


def subst_lambda(tree, sigma):
    items, tail = spine(tree)
    if len(items) < 3:
        return subst_generic(items, tail, sigma)
    tag, state, header, *cases = items
    declared, header = subst_header(header, sigma)
    inner = {key: value for key, value in sigma.items() if key not in declared}
    cases = [subst(case, inner) for case in cases]
    return rebuild([tag, subst(state, sigma), header, *cases], subst(tail, inner))


def subst_header(header, sigma):
    positions, tail = spine(header)
    if tail is not None:
        return set(), subst(header, sigma)
    declared, rebuilt = set(), []
    for position in positions:
        names, rest = spine(position)
        if rest is not None:
            rebuilt.append(subst(position, sigma))
            continue
        out = []
        for name in names:
            if is_identifier(name):
                declared.add(identity(name))
                out.append(name)
            else:
                out.append(subst(name, sigma))
        rebuilt.append(lst(out))
    return declared, lst(rebuilt)


def subst_binder(tree, sigma):
    items, tail = spine(tree)
    if len(items) != 3 or tail is not None:
        return subst_generic(items, tail, sigma)
    tag, clauses, body = items
    written, rest = spine(clauses)
    active = dict(sigma)
    out = []
    for clause in written:
        if isinstance(clause, Fork) and is_identifier(clause.left):
            out.append(Fork(clause.left, subst(clause.right, active)))
            active.pop(identity(clause.left), None)
        else:
            out.append(subst(clause, active))
    return lst([tag, rebuild(out, subst(rest, active)), subst(body, active)])


def subst_generic(items, tail, sigma):
    return rebuild([subst(item, sigma) for item in items], subst(tail, sigma))


# ---------------------------------------------------------------- store and intrinsics


class Store:
    def __init__(self):
        self.cells = {}
        self.next_identity = 0x8000
        self.next_address = 0

    def fresh_identity(self):
        key = self.next_identity.to_bytes(2, "big")
        self.next_identity += 1
        return key


def arguments(tree, count, name):
    items = elements(tree, f"the arguments of {name}")
    if len(items) != count:
        raise MachineError(f"{name} takes {count} arguments")
    return items


def address_key(tree):
    items, tail = spine(tree)
    if kind(tree) != b":address" or tail is not None or len(items) != 2:
        raise MachineError("expected an address")
    private = items[1]
    if not (isinstance(private, Fork) and private.left == b":private"):
        raise MachineError("expected an address")
    return private.right


def intrinsic_unique(argument, store):
    (name,) = arguments(argument, 1, "unique")
    if not isinstance(name, bytes):
        raise MachineError("unique expects a name fruit")
    return make_identifier(name, store.fresh_identity())


def intrinsic_fruit_equal(argument, store):
    first, second = arguments(argument, 2, "fruit-equal")
    if isinstance(first, bytes) and isinstance(second, bytes) and first == second:
        return b"true"
    return None


def intrinsic_new(argument, store):
    (value,) = arguments(argument, 1, "new")
    key = store.next_address.to_bytes(4, "big")
    store.next_address += 1
    store.cells[key] = value
    return lst([lst([b":address"]), Fork(b":private", key)])


def intrinsic_get(argument, store):
    (address,) = arguments(argument, 1, "get")
    key = address_key(address)
    if key not in store.cells:
        raise MachineError("unknown address")
    return store.cells[key]


def intrinsic_set(argument, store):
    address, value = arguments(argument, 2, "set")
    key = address_key(address)
    if key not in store.cells:
        raise MachineError("unknown address")
    store.cells[key] = value
    return None


INTRINSICS = {
    b"unique": intrinsic_unique,
    b"fruit-equal": intrinsic_fruit_equal,
    b"new": intrinsic_new,
    b"get": intrinsic_get,
    b"set": intrinsic_set,
}


def intrinsic(name):
    return lst([lst([b":intrinsic"]), name.encode()])


# ---------------------------------------------------------------- lambdas


def read_header(header):
    positions = []
    for position in elements(header, "a lambda header"):
        names = tuple(elements(position, "a header position"))
        if len(names) not in (1, 2):
            raise MachineError("a position binds one or two names")
        if not all(is_identifier(name) for name in names):
            raise MachineError("a declaration must be an identifier")
        positions.append(names)
    if not positions:
        raise MachineError("a lambda needs at least one parameter")
    keys = [identity(name) for names in positions for name in names]
    if len(keys) != len(set(keys)):
        raise MachineError("a header declares an identity twice")
    return positions


def read_state(state):
    if state is None:
        return 0, []
    if isinstance(state, Fork) and state.left == b":private" and isinstance(state.right, bytes):
        data = state.right
        return data[0], [CODE_SHAPE[code] for code in data[1:]]
    raise MachineError("malformed lambda private state")


def bind(names, argument):
    if len(names) == 1:
        return {identity(names[0]): argument}
    left, right = names
    match shape(argument):
        case b":fork":
            return {identity(left): argument.left, identity(right): argument.right}
        case b":fruit":
            return {identity(left): argument}
        case _:
            return {}


def select(positions, cases, shapes):
    rows = []
    for case in cases:
        if not isinstance(case, Fork):
            raise MachineError("a case is a pair of a shape row and an expression")
        row = elements(case.left, "a shape row")
        if not all(symbol in SHAPES for symbol in row):
            raise MachineError("a shape row holds shape symbols")
        if len(row) != len(positions):
            raise MachineError("a shape row must cover every parameter")
        for expected, names in zip(row, positions):
            if expected == b":fork" and len(names) == 1:
                raise MachineError("{:fork} needs a two-name position")
        rows.append((row, case.right))
    for row, body in rows:
        if all(expected in (b":all", actual) for expected, actual in zip(row, shapes)):
            return body
    raise MachineError("no case matches")


def apply(callee, argument, store):
    """One application: ('enter', body) for a selected case, ('return', value) otherwise."""
    if kind(callee) == b":intrinsic":
        items = elements(callee, "an intrinsic")
        operation = INTRINSICS.get(items[1]) if len(items) == 2 else None
        if operation is None:
            raise MachineError("unknown intrinsic")
        return "return", operation(argument, store)
    if kind(callee) != b":lambda":
        raise MachineError("expected a callable")
    items = elements(callee, "a lambda")
    if len(items) < 4:
        raise MachineError("a lambda needs a header and at least one case")
    tag, state, header, *cases = items
    positions = read_header(header)
    count, shapes = read_state(state)
    if count >= len(positions):
        raise MachineError("malformed lambda progress")
    sigma = bind(positions[count], argument)
    cases = [subst(case, sigma) for case in cases]
    shapes = shapes + [shape(argument)]
    count += 1
    if count < len(positions):
        data = bytes([count] + [SHAPE_CODE[s] for s in shapes])
        return "return", lst([tag, Fork(b":private", data), header, *cases])
    return "enter", select(positions, cases, shapes)


# ---------------------------------------------------------------- the machine


def step(mode, tree, stack, store):
    if mode == "enter":
        return ("run", tree) if kind(tree) == b":binder" else ("return", tree)
    if mode == "run":
        items = elements(tree, "a binder")
        if len(items) != 3:
            raise MachineError("a binder has clauses and one body")
        tag, clauses, body = items
        clauses = elements(clauses, "binder clauses")
        if not clauses:
            raise MachineError("a binder needs at least one clause")
        first = clauses[0]
        if not (isinstance(first, Fork) and is_identifier(first.left)):
            raise MachineError("a clause is a pair of a declaration and an expression")
        stack.append((tag, first.left, clauses[1:], body))
        return "force", first.right
    if mode == "force":
        if kind(tree) == b"$":
            items = elements(tree, "an application")
            if len(items) != 3:
                raise MachineError("an application has one callee and one operand")
            return apply(items[1], items[2], store)
        if kind(tree) == b":binder":
            return "run", tree
        if is_identifier(tree):
            raise MachineError("cannot run an unbound identifier")
        return "return", tree
    if mode == "return":
        tag, declaration, rest, body = stack.pop()
        remaining = subst_binder(lst([tag, lst(rest), body]), {identity(declaration): tree})
        _, clauses, body = elements(remaining, "a binder")
        if clauses is None:
            return "enter", body
        return "run", remaining
    raise AssertionError(mode)


def run(program, globals_=None, limit=10_000_000):
    """Run a program tree. `globals_` maps identity bytes to trees (intrinsics, a parsed
    source); entry rewrites the program with them. Returns (result, store, steps)."""
    store = Store()
    mode, tree = "enter", subst(program, globals_ or {})
    stack = []
    for steps in range(limit):
        if mode == "return" and not stack:
            return tree, store, steps
        mode, tree = step(mode, tree, stack, store)
    raise MachineError("step limit reached")


# ---------------------------------------------------------------- text


TOKEN = re.compile(r"\s*(~\{[0-9a-fA-F\s]*\}|\{[^{}]*\}|~\[|~|\[|\]|,)")


def parse(source):
    tokens = TOKEN.findall(source)
    if "".join(TOKEN.sub("", source).split()):
        raise ValueError("unexpected syntax")
    position = 0

    def take():
        nonlocal position
        position += 1
        return tokens[position - 1]

    def read(token):
        if token == "~":
            return None
        if token.startswith("~{"):
            return bytes.fromhex(token[2:-1])
        if token.startswith("{"):
            return token[1:-1].encode("utf-8")
        items = []
        following = take()
        while following != "]":
            items.append(read(following))
            following = take()
            if following == ",":
                following = take()
        if token == "~[":
            if len(items) != 2:
                raise ValueError("a pair has exactly two elements")
            return Fork(*items)
        return lst(items)

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
    items, tail = spine(tree, stop_at_identifiers=False)
    if tail is None:
        return "[" + ", ".join(map(display, items)) + "]"
    return "~[" + display(tree.left) + ", " + display(tree.right) + "]"
