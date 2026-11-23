"""Sketch: the heap and the identity table (proposal (23.11.26, 6)). Throwaway, runnable.

Only identities and environments are modelled; forks, fruits, CLOSURE records and long refs
follow the cells layout (19.11.26, 4), (19.11.26, 10), (22.11.26, 20), (22.11.26, 20).
"""

from dataclasses import dataclass, field

# ------------------------------------------------------------------ refs

Ref = int  # a 16-bit word: payload << 3 | kind

LEAF, FORK, UNKNOWN_FRUIT, INDIRECT, IDENTITY, KNOWN_FRUIT, CLOSURE, FREE = range(8)


def make_ref(kind: int, payload: int) -> Ref:
    assert payload < 1 << 13, "larger payloads go through a long ref"
    return payload << 3 | kind


def kind_of(ref: Ref) -> int:
    return ref & 0b111


def payload_of(ref: Ref) -> int:
    return ref >> 3


# ------------------------------------------------------------------ the heap


@dataclass
class IdentityRecord:
    run: int        # index into Heap.runs
    counter: int    # issued by the parser or `unique` within that run
    prime: int      # 0 as issued; raised by capture-avoiding renaming
    escaped: bool = False  # has appeared as data (`unique`, or exposed by taking a tree apart)


@dataclass
class Heap:
    cells: bytearray = field(default_factory=bytearray)  # forks, unknown fruits, CLOSUREs, long refs
    registry: bytes = b""                                # known fruits: retained, fixed per version
    runs: list[bytes] = field(default_factory=list)      # run ids; runs[0] is this run
    identities: list[IdentityRecord] = field(default_factory=list)  # the identity table
    interned: dict[tuple[int, int, int], int] = field(default_factory=dict)  # (run, counter, prime) -> index
    next_counter: int = 0


def new_heap(run_id: bytes) -> Heap:
    assert len(run_id) == 8
    return Heap(runs=[run_id])


# ------------------------------------------------------------------ identity operations


def intern(heap: Heap, run: int, counter: int, prime: int) -> Ref:
    """The one identity ref for (run, counter, prime); equal identities get equal refs."""
    key = (run, counter, prime)
    index = heap.interned.get(key)
    if index is None:
        index = len(heap.identities)
        heap.identities.append(IdentityRecord(run, counter, prime))
        heap.interned[key] = index
    return make_ref(IDENTITY, index)


def issue(heap: Heap) -> Ref:
    """A new identity in this run: the parser (per declaration) and `unique` (per call)."""
    counter = heap.next_counter
    heap.next_counter += 1
    return intern(heap, 0, counter, 0)


def record(heap: Heap, ref: Ref) -> IdentityRecord:
    assert kind_of(ref) == IDENTITY
    return heap.identities[payload_of(ref)]


def prime(heap: Heap, ref: Ref, avoid: set[Ref]) -> Ref:
    """Capture avoidance: the same base with the smallest higher prime not in `avoid`."""
    r = record(heap, ref)
    k = r.prime + 1
    while (candidate := intern(heap, r.run, r.counter, k)) in avoid:
        k += 1
    return candidate


def same(a: Ref, b: Ref) -> bool:
    """Identity equality is ref equality (short refs; a long ref is resolved to its index first)."""
    return a == b


def escape(heap: Heap, ref: Ref) -> None:
    """Mark an identity that may now occur as a hole: `unique`, or an identifier exposed as data."""
    record(heap, ref).escaped = True


def needs_capture_check(heap: Heap, declarations: list[Ref]) -> bool:
    """A scope needs the free-identity check only if it declares an escaped identity."""
    return any(record(heap, d).escaped for d in declarations)


# ------------------------------------------------------------------ bytes: inspection, persistence


def uleb(n: int) -> bytes:
    out = bytearray()
    while True:
        byte, n = n & 0x7F, n >> 7
        out.append(byte | (0x80 if n else 0))
        if not n:
            return bytes(out)


def read_uleb(data: bytes, at: int) -> tuple[int, int]:
    value = shift = 0
    while True:
        byte = data[at]
        at += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            return value, at


def identity_bytes(heap: Heap, ref: Ref) -> bytes:
    """What a program sees when it reads the identity as a fruit; what persistence writes."""
    r = record(heap, ref)
    return heap.runs[r.run] + uleb(r.counter) + uleb(r.prime)


def load_identity(heap: Heap, data: bytes) -> Ref:
    """Persistence: intern identity bytes read back from saved text."""
    run_id = data[:8]
    counter, at = read_uleb(data, 8)
    prime_, at = read_uleb(data, at)
    assert at == len(data)
    if run_id not in heap.runs:
        heap.runs.append(run_id)
    return intern(heap, heap.runs.index(run_id), counter, prime_)


# ------------------------------------------------------------------ environments

# Modelled as Python lists here; in cells an environment is a persistent list of
# ~[<identity ref>, <value ref>] forks, newest first (19.11.26, 20).
Env = list[tuple[Ref, Ref]]


def extend(env: Env, key: Ref, value: Ref) -> Env:
    return [(key, value)] + env  # shares the old tail in cells


def lookup(env: Env, key: Ref) -> Ref | None:
    for k, v in env:  # newest first: shadowing
        if same(k, key):  # a 16-bit compare per entry
            return v
    return None


# ------------------------------------------------------------------ self-check


if __name__ == "__main__":
    heap = new_heap(b"RUN-0001")
    x = issue(heap)                       # parser: a declaration of x
    x_again = intern(heap, 0, 0, 0)
    assert same(x, x_again)               # interning: one ref per identity
    y = issue(heap)
    x1 = prime(heap, x, avoid={y})        # x' : same base, prime 1
    assert not same(x1, x) and record(heap, x1).prime == 1
    x2 = prime(heap, x, avoid={x1})       # x'' when x' is in use nearby
    assert record(heap, x2).prime == 2
    saved = identity_bytes(heap, x1)      # persisted form
    other = new_heap(b"RUN-0002")
    loaded = load_identity(other, saved)  # a later run: a foreign run index, no collision
    assert record(other, loaded).run == 1 and record(other, issue(other)).run == 0
    env = extend(extend([], x, 100), x1, 200)
    assert lookup(env, x) == 100 and lookup(env, x1) == 200
    escape(heap, y)
    assert needs_capture_check(heap, [x, y]) and not needs_capture_check(heap, [x, x1])
    print("ok", len(heap.identities), "identities;", saved.hex(" "))
