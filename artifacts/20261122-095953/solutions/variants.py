"""Five variants of the reference machine, differing only in how declarations and usages are
told apart. Throwaway; see comparison.md.

All variants rewrite literally (22.11.26, 24) and share the steps of ../reference-machine/machine.py.

- base        identifiers in declaration slots declare; general shadowing (22.11.26, 26).
- roles       solution 1: a declaration is `~[{:decl}, ID]`, a usage `~[{:use}, ID]`, and a plain
              `ID` (what `unique` returns) is inert data. Only usages are rewritten; `{:decl}`
              slots declare and shadow.
- provenance  solution 2: rewriting inserts every fork-shaped value inside a seal. Written code has
              no shadowing (a repeat is a usage); a seal protects the identities declared by
              lambdas and binders inside it. Seals are invisible to inspection and display.
- fresh       solution 3: when a lambda is first applied or a binder starts running, every
              declaration in its tree gets a fresh identity, scoped like the counting rule (a
              repeat inside a declaration's scope is a usage). No shadowing.
- fresh+shadow  solution 3 with general shadowing kept.

A declaration slot is a name in a lambda header position or the left side of a binder clause.
"""

from dataclasses import dataclass
import sys

sys.path.insert(0, __file__.rsplit("/", 2)[0] + "/reference-machine")

from machine import (  # noqa: E402
    CODE_SHAPE, SHAPE_CODE, SHAPES, Fork, MachineError, lst, make_identifier, parse,
)
from machine import display as display_plain  # noqa: E402

sys.setrecursionlimit(50_000)


@dataclass(frozen=True)
class Seal:
    tree: object  # always a Fork


def seal(tree):
    return Seal(tree) if isinstance(tree, Fork) else tree


def unseal(tree):
    return tree.tree if isinstance(tree, Seal) else tree


def plain(tree):
    """The tree with all seals removed (what inspection and plain display see)."""
    tree = unseal(tree)
    if not isinstance(tree, Fork):
        return tree
    left, right = plain(tree.left), plain(tree.right)
    if left is tree.left and right is tree.right:
        return tree
    return Fork(left, right)


def display(tree):
    return display_plain(plain(tree))


def display_sealed(tree):
    """A persisted form that keeps provenance: a seal prints as #(...)."""
    if isinstance(tree, Seal):
        return "#(" + display_sealed(tree.tree) + ")"
    if not isinstance(tree, Fork):
        return display_plain(tree)
    return "~[" + display_sealed(tree.left) + ", " + display_sealed(tree.right) + "]"


@dataclass(frozen=True)
class Config:
    name: str
    roles: bool = False
    seals: bool = False
    rename: bool = False
    shadow: bool = True


VARIANTS = [
    Config("base"),
    Config("roles", roles=True),
    Config("provenance", seals=True, shadow=False),
    Config("fresh", rename=True, shadow=False),
    Config("fresh+shadow", rename=True, shadow=True),
]


class Store:
    def __init__(self):
        self.cells = {}
        self.next_identity = 0x8000
        self.next_address = 0

    def fresh_identity(self):
        key = self.next_identity.to_bytes(4, "big")
        self.next_identity += 1
        return key


class Machine:
    def __init__(self, config):
        self.c = config
        self._declared = {}

    # ------------------------------------------------------------ structure (seal-aware)

    @staticmethod
    def is_fork(tree):
        return isinstance(unseal(tree), Fork)

    @staticmethod
    def parts(tree):
        # Taking a sealed value apart exposes its own structure; nested seals stay.
        tree = unseal(tree)
        return tree.left, tree.right

    def kind(self, tree):
        if not self.is_fork(tree):
            return None
        head, _ = self.parts(tree)
        if not self.is_fork(head):
            return None
        name, rest = self.parts(head)
        if isinstance(name, bytes) and unseal(rest) is None:
            return name
        return None

    def raw_spine(self, tree):
        items = []
        while self.is_fork(tree):
            left, tree = self.parts(tree)
            items.append(left)
        return items, unseal(tree)

    def is_identifier(self, tree):
        if self.kind(tree) != b":id":
            return False
        items, tail = self.raw_spine(tree)
        if tail is not None or len(items) != 3 or not isinstance(items[1], bytes):
            return False
        private = unseal(items[2])
        return isinstance(private, Fork) and private.left == b":private" and isinstance(private.right, bytes)

    def identity(self, tree):
        items, _ = self.raw_spine(tree)
        return unseal(items[2]).right

    def name_of(self, tree):
        return self.raw_spine(tree)[0][1]

    def role(self, tree, role):
        if not self.is_fork(tree):
            return None
        left, right = self.parts(tree)
        if left == role and self.is_identifier(right):
            return self.identity(right)
        return None

    def occurrence(self, tree):
        """The identity a rewrite replaces at this node, if any."""
        if self.c.roles:
            return self.role(tree, b":use")
        return self.identity(tree) if self.is_identifier(tree) else None

    def declaration(self, tree):
        """The identity a declaration slot declares, if any."""
        if self.c.roles:
            return self.role(tree, b":decl")
        return self.identity(tree) if self.is_identifier(tree) else None

    def is_name(self, tree):
        return self.is_identifier(tree) or (
            self.c.roles and (self.role(tree, b":use") is not None or self.role(tree, b":decl") is not None)
        )

    def spine(self, tree):
        """List cells as written; a name standing as the tail is not more cells."""
        items = []
        while self.is_fork(tree) and not self.is_name(tree):
            left, tree = self.parts(tree)
            items.append(left)
        return items, tree

    def elements(self, tree, what):
        items, tail = self.spine(tree)
        if unseal(tail) is not None:
            raise MachineError(f"{what} must be a proper list")
        return items

    @staticmethod
    def shape(tree):
        tree = unseal(tree)
        if tree is None:
            return b":leaf"
        return b":fork" if isinstance(tree, Fork) else b":fruit"

    # ------------------------------------------------------------ rewriting

    def insert(self, value):
        return seal(value) if self.c.seals else value

    def subst(self, tree, sigma):
        if not sigma or not self.is_fork(tree):
            return tree
        if isinstance(tree, Seal):
            protected = self.declared_within(tree.tree)
            inner = {k: v for k, v in sigma.items() if k not in protected}
            if not inner:
                return tree
            out = self.subst(tree.tree, inner)
            return tree if out is tree.tree else seal(out)
        found = self.occurrence(tree)
        if found is not None:
            return self.insert(sigma[found]) if found in sigma else tree
        if self.is_name(tree):
            return tree  # a plain id or a declaration node (roles) is inert
        if self.c.shadow and self.kind(tree) == b":lambda":
            return self.subst_lambda(tree, sigma)
        if self.c.shadow and self.kind(tree) == b":binder":
            return self.subst_binder(tree, sigma)
        left, right = self.subst(tree.left, sigma), self.subst(tree.right, sigma)
        return tree if (left is tree.left and right is tree.right) else Fork(left, right)

    def rebuild(self, items, tail):
        for item in reversed(items):
            tail = Fork(item, tail)
        return tail

    def subst_lambda(self, tree, sigma):
        items, tail = self.spine(tree)
        if len(items) < 3:
            return self.rebuild([self.subst(i, sigma) for i in items], self.subst(tail, sigma))
        tag, state, header, *cases = items
        positions, rest = self.spine(header)
        declared = set()
        if unseal(rest) is None:
            rebuilt = []
            for position in positions:
                names, ptail = self.spine(position)
                if unseal(ptail) is not None:
                    rebuilt.append(self.subst(position, sigma))
                    continue
                out = []
                for name in names:
                    d = self.declaration(name)
                    if d is not None:
                        declared.add(d)
                        out.append(name)
                    else:
                        out.append(self.subst(name, sigma))
                rebuilt.append(lst(out))
            header = lst(rebuilt)
        else:
            header = self.subst(header, sigma)
        inner = {k: v for k, v in sigma.items() if k not in declared}
        return self.rebuild([tag, state, header, *[self.subst(c, inner) for c in cases]],
                            self.subst(tail, inner))

    def subst_binder(self, tree, sigma):
        items, tail = self.spine(tree)
        if len(items) != 3 or unseal(tail) is not None:
            return self.rebuild([self.subst(i, sigma) for i in items], self.subst(tail, sigma))
        tag, clauses, body = items
        written, rest = self.spine(clauses)
        active = dict(sigma)
        out = []
        for clause in written:
            if self.is_fork(clause):
                left, right = self.parts(clause)
                d = self.declaration(left)
                if d is not None:
                    out.append(Fork(left, self.subst(right, active)))
                    active.pop(d, None)
                    continue
            out.append(self.subst(clause, active))
        return lst([tag, self.rebuild(out, self.subst(rest, active)), self.subst(body, active)])

    def declared_within(self, tree):
        """Identities declared by lambdas and binders anywhere inside `tree` (provenance)."""
        key = id(tree)
        cached = self._declared.get(key)
        if cached is not None and cached[0] is tree:
            return cached[1]
        found = set()
        stack = [tree]
        while stack:
            node = unseal(stack.pop())
            if not isinstance(node, Fork):
                continue
            kind = self.kind(node)
            if kind == b":lambda":
                items, _ = self.spine(node)
                if len(items) >= 3:
                    for position in self.spine(items[2])[0]:
                        for name in self.spine(position)[0]:
                            d = self.declaration(name)
                            if d is not None:
                                found.add(d)
            elif kind == b":binder":
                items, _ = self.spine(node)
                if len(items) == 3:
                    for clause in self.spine(items[1])[0]:
                        if self.is_fork(clause):
                            d = self.declaration(self.parts(clause)[0])
                            if d is not None:
                                found.add(d)
            if not self.is_identifier(node):
                stack.extend((node.left, node.right))
        self._declared[key] = (tree, found)
        return found

    def rename(self, tree, mapping, store):
        """Fresh identities for every declaration in `tree`; a repeat in scope is a usage."""
        if not isinstance(tree, Fork):
            return tree
        if self.is_identifier(tree):
            key = self.identity(tree)
            if key in mapping:
                return make_identifier(self.name_of(tree), mapping[key])
            return tree
        kind = self.kind(tree)
        if kind == b":lambda":
            items, tail = self.spine(tree)
            if len(items) >= 3:
                new = {}
                for position in self.spine(items[2])[0]:
                    for name in self.spine(position)[0]:
                        if self.is_identifier(name):
                            d = self.identity(name)
                            if d not in mapping and d not in new:
                                new[d] = store.fresh_identity()
                inner = mapping | new
                return self.rebuild([self.rename(i, inner, store) for i in items],
                                    self.rename(tail, inner, store))
        if kind == b":binder":
            items, tail = self.spine(tree)
            if len(items) == 3 and tail is None:
                tag, clauses, body = items
                written, rest = self.spine(clauses)
                scope = mapping
                out = []
                for clause in written:
                    if isinstance(clause, Fork) and self.is_identifier(clause.left):
                        right = self.rename(clause.right, scope, store)
                        d = self.identity(clause.left)
                        if d not in scope:
                            scope = scope | {d: store.fresh_identity()}
                        out.append(Fork(self.rename(clause.left, scope, store), right))
                    else:
                        out.append(self.rename(clause, scope, store))
                return lst([self.rename(tag, mapping, store),
                            self.rebuild(out, self.rename(rest, scope, store)),
                            self.rename(body, scope, store)])
        left, right = self.rename(tree.left, mapping, store), self.rename(tree.right, mapping, store)
        return tree if (left is tree.left and right is tree.right) else Fork(left, right)

    # ------------------------------------------------------------ intrinsics

    def arguments(self, tree, count, name):
        items = self.elements(tree, f"the arguments of {name}")
        if len(items) != count:
            raise MachineError(f"{name} takes {count} arguments")
        return items

    def address_key(self, tree):
        items, tail = self.spine(tree)
        if self.kind(tree) != b":address" or unseal(tail) is not None or len(items) != 2:
            raise MachineError("expected an address")
        return plain(items[1]).right

    def intrinsic(self, name, argument, store):
        if name == b"unique":
            (n,) = self.arguments(argument, 1, "unique")
            if not isinstance(n, bytes):
                raise MachineError("unique expects a name fruit")
            return make_identifier(n, store.fresh_identity())
        if name == b"fruit-equal":
            a, b = (unseal(x) for x in self.arguments(argument, 2, "fruit-equal"))
            return b"true" if isinstance(a, bytes) and a == b else None
        if name == b"new":
            (v,) = self.arguments(argument, 1, "new")
            key = store.next_address.to_bytes(4, "big")
            store.next_address += 1
            store.cells[key] = v
            return lst([lst([b":address"]), Fork(b":private", key)])
        if name == b"get":
            (a,) = self.arguments(argument, 1, "get")
            return store.cells[self.address_key(a)]
        if name == b"set":
            a, v = self.arguments(argument, 2, "set")
            store.cells[self.address_key(a)] = v
            return None
        raise MachineError("unknown intrinsic")

    # ------------------------------------------------------------ lambdas

    def read_header(self, header):
        positions = []
        for position in self.elements(header, "a lambda header"):
            names = self.elements(position, "a header position")
            if len(names) not in (1, 2):
                raise MachineError("a position binds one or two names")
            keys = tuple(self.declaration(n) for n in names)
            if None in keys:
                raise MachineError("a declaration must be an identifier")
            positions.append(keys)
        if not positions:
            raise MachineError("a lambda needs at least one parameter")
        flat = [k for p in positions for k in p]
        if len(flat) != len(set(flat)):
            raise MachineError("a header declares an identity twice")
        return positions

    @staticmethod
    def read_state(state):
        state = plain(state)
        if state is None:
            return 0, []
        if isinstance(state, Fork) and state.left == b":private" and isinstance(state.right, bytes):
            return state.right[0], [CODE_SHAPE[c] for c in state.right[1:]]
        raise MachineError("malformed lambda private state")

    def bind(self, keys, argument):
        if len(keys) == 1:
            return {keys[0]: argument}
        left, right = keys
        shape = self.shape(argument)
        if shape == b":fork":
            l, r = self.parts(argument)
            return {left: l, right: r}
        if shape == b":fruit":
            return {left: argument}
        return {}

    def select(self, positions, cases, shapes):
        rows = []
        for case in cases:
            if not self.is_fork(case):
                raise MachineError("a case is a pair of a shape row and an expression")
            row_tree, body = self.parts(case)
            row = [unseal(s) for s in self.elements(row_tree, "a shape row")]
            if not all(s in SHAPES for s in row):
                raise MachineError("a shape row holds shape symbols")
            if len(row) != len(positions):
                raise MachineError("a shape row must cover every parameter")
            for expected, keys in zip(row, positions):
                if expected == b":fork" and len(keys) == 1:
                    raise MachineError("{:fork} needs a two-name position")
            rows.append((row, body))
        for row, body in rows:
            if all(e in (b":all", a) for e, a in zip(row, shapes)):
                return body
        raise MachineError("no case matches")

    def apply(self, callee, argument, store):
        kind = self.kind(callee)
        if kind == b":intrinsic":
            items = self.elements(callee, "an intrinsic")
            return "return", self.intrinsic(unseal(items[1]), argument, store)
        if kind != b":lambda":
            raise MachineError("expected a callable")
        items = self.elements(callee, "a lambda")
        if len(items) < 4:
            raise MachineError("a lambda needs a header and at least one case")
        count, shapes = self.read_state(items[1])
        if self.c.rename and count == 0:
            callee = self.rename(plain(callee), {}, store)
            items = self.elements(callee, "a lambda")
        tag, state, header, *cases = items
        positions = self.read_header(header)
        if count >= len(positions):
            raise MachineError("malformed lambda progress")
        sigma = self.bind(positions[count], argument)
        cases = [self.subst(case, sigma) for case in cases]
        shapes = shapes + [self.shape(argument)]
        count += 1
        if count < len(positions):
            data = bytes([count] + [SHAPE_CODE[s] for s in shapes])
            return "return", lst([tag, Fork(b":private", data), header, *cases])
        return "enter", self.select(positions, cases, shapes)

    # ------------------------------------------------------------ the machine

    def step(self, mode, tree, stack, store):
        if mode == "enter":
            return ("run", tree) if self.kind(tree) == b":binder" else ("return", tree)
        if mode in ("run", "continue"):
            if mode == "run" and self.c.rename:
                tree = self.rename(plain(tree), {}, store)
            items = self.elements(tree, "a binder")
            if len(items) != 3:
                raise MachineError("a binder has clauses and one body")
            tag, clauses, body = items
            clauses = self.elements(clauses, "binder clauses")
            if not clauses:
                raise MachineError("a binder needs at least one clause")
            if not self.is_fork(clauses[0]):
                raise MachineError("a clause is a pair of a declaration and an expression")
            left, right = self.parts(clauses[0])
            key = self.declaration(left)
            if key is None:
                raise MachineError("a declaration must be an identifier")
            stack.append((tag, key, clauses[1:], body))
            return "force", right
        if mode == "force":
            kind = self.kind(tree)
            if kind == b"$":
                items = self.elements(tree, "an application")
                if len(items) != 3:
                    raise MachineError("an application has one callee and one operand")
                return self.apply(items[1], items[2], store)
            if kind == b":binder":
                return "run", tree
            if self.occurrence(tree) is not None:
                raise MachineError("cannot run an unbound identifier")
            return "return", tree
        if mode == "return":
            tag, key, rest, body = stack.pop()
            sigma = {key: tree}
            if self.c.shadow:
                remaining = self.subst_binder(lst([tag, lst(rest), body]), sigma)
                _, clauses, body = self.elements(remaining, "a binder")
            else:
                clauses = [self.subst(c, sigma) for c in rest]
                body = self.subst(body, sigma)
                remaining = lst([tag, lst(clauses), body])
                clauses = clauses or None
            if not clauses:
                return "enter", body
            return "continue", remaining
        raise AssertionError(mode)

    def run(self, program, globals_, limit=2_000_000):
        store = Store()
        mode, tree = "enter", self.subst(program, globals_)
        stack = []
        for steps in range(limit):
            if mode == "return" and not stack:
                return tree, store, steps
            mode, tree = self.step(mode, tree, stack, store)
        raise MachineError("step limit reached")


def intrinsic_tree(name):
    return lst([lst([b":intrinsic"]), name.encode()])


GLOBAL_KEYS = {"unique": b"\xf0", "fruit-equal": b"\xf1", "new": b"\xf3", "get": b"\xf4", "set": b"\xf5"}


def globals_for():
    return {key: intrinsic_tree(name) for name, key in GLOBAL_KEYS.items()}


def to_roles(machine, tree, slot=False):
    """Lower a program written with plain identifiers into the roles encoding: identifiers in
    declaration slots become `~[{:decl}, ID]`, all others `~[{:use}, ID]`. A hand-written
    `~[{:decl}, x]` or `~[{:use}, x]` conses the role onto the value of `x`."""
    if not isinstance(tree, Fork):
        return tree
    if machine.is_identifier(tree):
        return Fork(b":decl" if slot else b":use", tree)
    if tree.left in (b":decl", b":use") and machine.is_identifier(tree.right):
        return Fork(tree.left, Fork(b":use", tree.right))
    kind = machine.kind(tree)
    if kind == b":lambda":
        items, tail = machine.spine(tree)
        if len(items) >= 3:
            header = items[2]
            positions, ptail = machine.spine(header)
            if ptail is None:
                header = lst([
                    lst([to_roles(machine, n, True) for n in machine.spine(p)[0]])
                    if machine.spine(p)[1] is None else to_roles(machine, p)
                    for p in positions
                ])
            else:
                header = to_roles(machine, header)
            out = [to_roles(machine, items[0]), to_roles(machine, items[1]), header,
                   *[to_roles(machine, c) for c in items[3:]]]
            return machine.rebuild(out, to_roles(machine, tail))
    if kind == b":binder":
        items, tail = machine.spine(tree)
        if len(items) == 3 and tail is None:
            written, rest = machine.spine(items[1])
            clauses = [Fork(to_roles(machine, c.left, True), to_roles(machine, c.right))
                       if isinstance(c, Fork) else to_roles(machine, c) for c in written]
            return lst([to_roles(machine, items[0]), machine.rebuild(clauses, to_roles(machine, rest)),
                        to_roles(machine, items[2])])
    return Fork(to_roles(machine, tree.left, False), to_roles(machine, tree.right, False))
