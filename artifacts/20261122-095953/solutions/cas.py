"""The current scheme as a variant of the reference machine: capture-avoiding substitution with
private priming (23.11.26, 0), and unset names as an execution error (proposed). Throwaway.

- Shadowing: rewriting stops at a lambda or binder that declares the identity being substituted.
- Capture avoidance: when values are rewritten into the scope of a declaration whose identity
  occurs free in one of them, that declaration is primed first, within its own scope. This
  applies to nested lambdas and binders reached by a rewrite, to the later clauses of a running
  binder, and to the remaining parameters of the lambda being applied.
- Priming: the primed identity keeps its name and base and adds a prime count, the smallest one
  not in use nearby: identity bytes `<base> 0xfe <prime as ULEB128>` (the marker byte is a
  throwaway choice; the scheme only needs base and prime to be recoverable).
- Unset names: when a case is selected, a name its pattern left unset for the actual shape (a
  leaf sets neither name of a two-name position, a fruit only the first) must not occur free in
  the selected body.
"""

from variants import Config, Fork, Machine, MachineError, lst, make_identifier, unseal

MARK = b"\xfe"


def uleb(n):
    out = bytearray()
    while True:
        byte, n = n & 0x7F, n >> 7
        out.append(byte | (0x80 if n else 0))
        if not n:
            return bytes(out)


CAS = Config("cas")


class CasMachine(Machine):
    def __init__(self, config=CAS):
        super().__init__(config)
        self._free = {}
        self._ids = {}
        self.primed = {}  # primed identity -> (base, prime)

    # ------------------------------------------------------------ free identities

    def free(self, tree):
        if not isinstance(tree, Fork):
            return frozenset()
        cached = self._free.get(id(tree))
        if cached is not None and cached[0] is tree:
            return cached[1]
        if self.is_identifier(tree):
            result = frozenset([self.identity(tree)])
        elif self.kind(tree) == b":lambda" and len(self.spine(tree)[0]) >= 3:
            items, tail = self.spine(tree)
            declared, loose = self.header_declarations(items[2])
            inner = set()
            for item in items[3:] + [tail]:
                inner |= self.free(item)
            result = frozenset((inner - declared) | loose | self.free(items[0]) | self.free(items[1]))
        elif self.kind(tree) == b":binder" and self.binder_parts(tree) is not None:
            tag, written, rest, body = self.binder_parts(tree)
            out, declared = set(self.free(tag)), set()
            for clause in written:
                if isinstance(clause, Fork) and self.is_identifier(clause.left):
                    out |= self.free(clause.right) - declared
                    declared.add(self.identity(clause.left))
                else:
                    out |= self.free(clause) - declared
            out |= (self.free(rest) | self.free(body)) - declared
            result = frozenset(out)
        else:
            result = self.free(tree.left) | self.free(tree.right)
        self._free[id(tree)] = (tree, result)
        return result

    def all_identities(self, tree):
        if not isinstance(tree, Fork):
            return frozenset()
        cached = self._ids.get(id(tree))
        if cached is not None and cached[0] is tree:
            return cached[1]
        if self.is_identifier(tree):
            result = frozenset([self.identity(tree)])
        else:
            result = self.all_identities(tree.left) | self.all_identities(tree.right)
        self._ids[id(tree)] = (tree, result)
        return result

    def header_declarations(self, header):
        """(declared identities, free identities of non-declaration parts) of a header."""
        declared, loose = set(), set()
        positions, tail = self.spine(header)
        if tail is not None:
            return declared, set(self.free(header))
        for position in positions:
            names, ptail = self.spine(position)
            if ptail is not None:
                loose |= self.free(position)
                continue
            for name in names:
                if self.is_identifier(name):
                    declared.add(self.identity(name))
                else:
                    loose |= self.free(name)
        return declared, loose

    def binder_parts(self, tree):
        items, tail = self.spine(tree)
        if len(items) != 3 or tail is not None:
            return None
        written, rest = self.spine(items[1])
        return items[0], written, rest, items[2]

    # ------------------------------------------------------------ priming

    def prime(self, key, avoid):
        base, count = self.primed.get(key, (key, 0))
        while True:
            count += 1
            candidate = base + MARK + uleb(count)
            if candidate not in avoid:
                self.primed[candidate] = (base, count)
                return candidate

    def renaming(self, captured, names, avoid):
        """Identity -> primed identifier tree, for each captured declaration."""
        out, used = {}, set(avoid)
        for key in captured:
            new = self.prime(key, used)
            used.add(new)
            out[key] = make_identifier(names[key], new)
        return out

    def values_free(self, sigma):
        out = set()
        for value in sigma.values():
            out |= self.free(value)
        return out

    def names_in_header(self, header):
        names = {}
        for position in self.spine(header)[0]:
            for name in self.spine(position)[0]:
                if self.is_identifier(name):
                    names[self.identity(name)] = self.name_of(name)
        return names

    # ------------------------------------------------------------ rewriting

    def subst_lambda(self, tree, sigma):
        items, tail = self.spine(tree)
        if len(items) < 3:
            return super().subst_lambda(tree, sigma)
        header = items[2]
        declared, _ = self.header_declarations(header)
        inner = {k: v for k, v in sigma.items() if k not in declared}
        captured = declared & self.values_free(inner)
        if captured:
            rho = self.renaming(captured, self.names_in_header(header),
                                self.values_free(inner) | self.all_identities(tree))
            tree = self.prime_lambda(tree, rho)
        return super().subst_lambda(tree, sigma)

    def prime_lambda(self, tree, rho):
        """Rename the lambda's own declarations in `rho`: header slots and its scope."""
        items, tail = self.spine(tree)
        tag, state, header, *cases = items
        positions = []
        for position in self.spine(header)[0]:
            names = [rho.get(self.identity(n), n) if self.is_identifier(n) else n
                     for n in self.spine(position)[0]]
            positions.append(lst(names))
        cases = [Machine.subst(self, c, rho) for c in cases]
        return self.rebuild([tag, state, lst(positions), *cases], Machine.subst(self, tail, rho))

    def subst_binder(self, tree, sigma):
        parts = self.binder_parts(tree)
        if parts is None:
            return super().subst_binder(tree, sigma)
        tag, written, rest, body = parts
        active = dict(sigma)
        out = []
        index = 0
        while index < len(written):
            clause = written[index]
            if isinstance(clause, Fork) and self.is_identifier(clause.left):
                key = self.identity(clause.left)
                active.pop(key, None)
                if key in self.values_free(active):
                    # Prime this declaration in its scope: its own left side, later clauses, body.
                    rho = self.renaming({key}, {key: self.name_of(clause.left)},
                                        self.values_free(active) | self.all_identities(tree))
                    renamed = Machine.subst(self, lst([tag, lst(written[index + 1:]), body]), rho)
                    _, later, body = self.spine(renamed)[0]
                    written = written[:index] + [Fork(rho[key], clause.right)] + self.spine(later)[0]
                    clause = written[index]
                out.append(Fork(clause.left, Machine.subst(self, clause.right, self._before(sigma, written, index))))
            else:
                out.append(Machine.subst(self, clause, self._before(sigma, written, index)))
            index += 1
        final = self._before(sigma, written, len(written))
        return lst([tag, self.rebuild(out, Machine.subst(self, rest, final)), Machine.subst(self, body, final)])

    def _before(self, sigma, written, index):
        """`sigma` without the identities declared by clauses before `index`."""
        out = dict(sigma)
        for clause in written[:index]:
            if isinstance(clause, Fork) and self.is_identifier(clause.left):
                out.pop(self.identity(clause.left), None)
        return out

    # ------------------------------------------------------------ application

    def apply(self, callee, argument, store):
        if self.kind(callee) != b":lambda":
            return super().apply(callee, argument, store)
        items = self.elements(callee, "a lambda")
        if len(items) < 4:
            raise MachineError("a lambda needs a header and at least one case")
        tag, state, header, *cases = items
        positions = self.read_header(header)
        count, shapes = self.read_state(state)
        if count >= len(positions):
            raise MachineError("malformed lambda progress")
        sigma = self.bind(positions[count], argument)
        # The lambda's other declarations scope over the cases the argument is rewritten into.
        others = {k for p in positions for k in p} - set(sigma)
        captured = others & self.values_free(sigma)
        if captured:
            rho = self.renaming(captured, self.names_in_header(header),
                                self.values_free(sigma) | self.all_identities(callee))
            callee = self.prime_lambda(lst(items), rho)
            tag, state, header, *cases = self.elements(callee, "a lambda")
            positions = self.read_header(header)
        cases = [self.subst(case, sigma) for case in cases]
        shapes = shapes + [self.shape(argument)]
        count += 1
        if count < len(positions):
            from variants import SHAPE_CODE
            data = bytes([count] + [SHAPE_CODE[s] for s in shapes])
            return "return", lst([tag, Fork(b":private", data), header, *cases])
        return "enter", self.select_checked(positions, cases, shapes)

    def select_checked(self, positions, cases, shapes):
        body = self.select(positions, cases, shapes)
        unset = set()
        for keys, shape in zip(positions, shapes):
            if len(keys) == 2:
                if shape == b":leaf":
                    unset |= set(keys)
                elif shape == b":fruit":
                    unset.add(keys[1])
        if unset & self.free(unseal(body)):
            raise MachineError("a name is not bound for this shape")
        return body


class SimultaneousCasMachine(CasMachine):
    """CasMachine whose binders substitute simultaneously: the values bound so far accumulate and
    are applied together to each clause when it is forced, and to the body at the end, as an
    environment machine would. Sequential substitution let the order of clauses leak (E2)."""

    def step(self, mode, tree, stack, store):
        if mode in ("run", "continue"):
            if mode == "run":
                parts = self.binder_parts(tree)
                if parts is None or parts[2] is not None or not parts[1]:
                    return super().step(mode, tree, stack, store)
                tag, written, _, body = parts
                sigma = {}
            else:
                tag, written, body, sigma = tree
            clause = written[0]
            if not (isinstance(clause, Fork) and self.is_identifier(clause.left)):
                raise MachineError("a clause is a pair of a declaration and an expression")
            key = self.identity(clause.left)
            stack.append(("sim", tag, clause.left, written[1:], body, sigma))
            return "force", self.subst(clause.right, sigma)
        if mode == "return" and stack and stack[-1][0] == "sim":
            _, tag, declaration, rest, body, sigma = stack.pop()
            key = self.identity(declaration)
            sigma = {k: v for k, v in sigma.items() if k != key}  # shadowed from here on
            if key in self.values_free(sigma):
                # Earlier values carry a hole with this identity: prime this declaration in its scope.
                rho = self.renaming({key}, {key: self.name_of(declaration)},
                                    self.values_free(sigma) | self.all_identities(lst(rest)) | self.all_identities(body))
                renamed = Machine.subst(self, lst([tag, lst(rest), body]), rho)
                _, later, body = self.spine(renamed)[0]
                rest = self.spine(later)[0]
                key = self.identity(rho[key])
            sigma = sigma | {key: tree}
            if not rest:
                return "enter", self.subst(body, sigma)
            return "continue", (tag, rest, body, sigma)
        return super().step(mode, tree, stack, store)


from dataclasses import dataclass as _dataclass  # noqa: E402

from variants import SHAPE_CODE, Seal  # noqa: E402


@_dataclass(frozen=True)
class Partial(Seal):
    """A partial application: the lambda's tree with updated private state, plus the bindings so
    far, kept aside as an environment machine keeps them. Inspection sees only `tree`."""
    pending: tuple = ()  # ((identity, value), ...)


class SaturatingCasMachine(SimultaneousCasMachine):
    """Also substitutes a lambda's arguments together at saturation, not one by one."""

    def subst(self, tree, sigma):
        if isinstance(tree, Partial):
            if not sigma:
                return tree
            pending = dict(tree.pending)
            inner = {k: v for k, v in sigma.items() if k not in pending}
            new_tree = self.subst(tree.tree, inner)
            renamed = self._renamed_declarations(tree.tree, new_tree)
            pending = {renamed.get(k, k): self.subst(v, inner) for k, v in pending.items()}
            return Partial(new_tree, tuple(pending.items()))
        return super().subst(tree, sigma)

    def _renamed_declarations(self, old, new):
        old_names = list(self.names_in_header(self.spine(old)[0][2]))
        new_names = list(self.names_in_header(self.spine(new)[0][2]))
        return {a: b for a, b in zip(old_names, new_names) if a != b}

    def free(self, tree):
        if isinstance(tree, Partial):
            out = set(super().free(tree.tree))
            for _, value in tree.pending:
                out |= super().free(value) if not isinstance(value, Partial) else self.free(value)
            return frozenset(out)
        return super().free(unseal(tree)) if isinstance(tree, Seal) else super().free(tree)

    def all_identities(self, tree):
        if isinstance(tree, Partial):
            out = set(super().all_identities(tree.tree))
            for _, value in tree.pending:
                out |= self.all_identities(value)
            return frozenset(out)
        return super().all_identities(unseal(tree)) if isinstance(tree, Seal) else super().all_identities(tree)

    def apply(self, callee, argument, store):
        if self.kind(callee) != b":lambda":
            return super().apply(callee, argument, store)
        pending = dict(callee.pending) if isinstance(callee, Partial) else {}
        items = self.elements(callee, "a lambda")
        if len(items) < 4:
            raise MachineError("a lambda needs a header and at least one case")
        tag, state, header, *cases = items
        positions = self.read_header(header)
        count, shapes = self.read_state(state)
        if count >= len(positions):
            raise MachineError("malformed lambda progress")
        binding = self.bind(positions[count], argument)
        # Declarations still to be bound would capture free identities of this argument.
        remaining = {k for p in positions[count + 1:] for k in p}
        captured = remaining & self.values_free(binding)
        if captured:
            rho = self.renaming(captured, self.names_in_header(header),
                                self.values_free(binding) | self.values_free(pending) | self.all_identities(unseal(callee)))
            primed = self.prime_lambda(lst(items), rho)
            tag, state, header, *cases = self.elements(primed, "a lambda")
            positions = self.read_header(header)
        pending = pending | binding
        shapes = shapes + [self.shape(argument)]
        count += 1
        if count < len(positions):
            data = bytes([count] + [SHAPE_CODE[s] for s in shapes])
            return "return", Partial(lst([tag, Fork(b":private", data), header, *cases]), tuple(pending.items()))
        cases = [self.subst(case, pending) for case in cases]
        return "enter", self.select_checked(positions, cases, shapes)
