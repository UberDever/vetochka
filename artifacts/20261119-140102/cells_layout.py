#!/usr/bin/env python3
"""Exercise the packed cells proposal (19.11.26, 4) with real buffer bytes."""

import argparse
import mmap
from dataclasses import dataclass


LEAF, FORK, FRUIT, INDIRECT, INLINE_FRUIT = range(5)
KIND_BITS = 3
KIND_MASK = (1 << KIND_BITS) - 1


@dataclass(frozen=True)
class Fork:
    offset: int
    base: int


def uleb128(number):
    result = bytearray()
    while number >= 128:
        result.append((number & 127) | 128)
        number >>= 7
    result.append(number)
    return bytes(result)


class Buffer:
    def __init__(self, width, data=None):
        self.width = width
        self.fork_size = 2 * width
        self.ref_range = 1 << (8 * width - KIND_BITS)
        self.data = bytearray() if data is None else data

    def ref(self, kind, offset, base):
        displacement = offset - base
        if not 0 <= displacement < self.ref_range:
            raise ValueError("target is outside the current ref window")
        return ((displacement << KIND_BITS) | kind).to_bytes(self.width, "little")

    def long_ref(self, kind, offset, location):
        word = ((offset - location) << KIND_BITS) | kind
        return word.to_bytes(2 * self.width, "little", signed=True)

    def inline_fruit(self, payload):
        if len(payload) > min(7, self.width - 1):
            raise ValueError("fruit does not fit in the ref")
        word = bytes([(len(payload) << KIND_BITS) | INLINE_FRUIT]) + payload
        return word.ljust(self.width, b"\0")

    def read(self, offset, size):
        if not (0 <= offset <= offset + size <= len(self.data)):
            raise ValueError(f"out of bounds at 0x{offset:X}, size {size}")
        return self.data[offset : offset + size]

    def fruit(self, offset):
        length = shift = 0
        while True:
            byte = self.read(offset, 1)[0]
            offset += 1
            length |= (byte & 127) << shift
            if byte < 128:
                return bytes(self.read(offset, length))
            shift += 7

    def resolve(self, slot, base):
        raw = self.read(slot, self.width)
        kind = raw[0] & KIND_MASK
        if kind == LEAF:
            if any(raw):
                raise ValueError("leaf word must be zero")
            return None, ()
        if kind == INLINE_FRUIT:
            length = (raw[0] >> KIND_BITS) & 7
            if raw[0] & 0xC0 or length > self.width - 1:
                raise ValueError("invalid in-place fruit")
            return bytes(raw[1 : 1 + length]), ()

        target = base + (int.from_bytes(raw, "little") >> KIND_BITS)
        chain = []
        seen = set()
        while kind == INDIRECT:
            if target in seen:
                raise ValueError(f"cycle in long refs at 0x{target:X}")
            seen.add(target)
            chain.append(target)
            base = target
            raw = self.read(target, 2 * self.width)
            kind = raw[0] & KIND_MASK
            target += int.from_bytes(raw, "little", signed=True) >> KIND_BITS

        if kind == FORK:
            self.read(target, self.fork_size)
            return Fork(target, base), tuple(chain)
        if kind == FRUIT:
            return self.fruit(target), tuple(chain)
        raise ValueError(f"invalid target kind {kind}")

    def dump(self, offset, size, label):
        raw = self.read(offset, size).hex(" ").upper()
        print(f"{offset:08X}  {raw:<29}  {label}")


def dense_spine(width, count):
    """Build ~[left, rest] forks in order. Every tenth left is ~{61}."""
    heap = Buffer(width)
    leaf = bytes(width)
    fruit = heap.inline_fruit(b"a")
    base = bridges = 0
    for index in range(count):
        left = fruit if index % 10 == 0 else leaf
        next_offset = len(heap.data) + heap.fork_size
        if index == count - 1:
            heap.data.extend(left + leaf)
        elif next_offset + heap.fork_size - base >= heap.ref_range:
            # Leave room to name the long ref while still in the old window.
            heap.data.extend(left + heap.ref(INDIRECT, next_offset, base))
            base = next_offset
            heap.data.extend(heap.long_ref(FORK, base + heap.fork_size, base))
            bridges += 1
        else:
            heap.data.extend(left + heap.ref(FORK, next_offset, base))

    fork = Fork(0, 0)
    hops = 0
    for index in range(count):
        assert isinstance(fork, Fork)
        left, left_hops = heap.resolve(fork.offset, fork.base)
        assert left == (b"a" if index % 10 == 0 else None) and not left_hops
        fork, right_hops = heap.resolve(fork.offset + width, fork.base)
        hops += len(right_hops)
    assert fork is None and hops == bridges
    assert len(heap.data) == (count + bridges) * heap.fork_size
    print(f"w={width}: {count:,} forks, {bridges:,} long refs, "
          f"{len(heap.data):,} bytes, {hops:,} extra lookups")


def shared_trees():
    """~[~[s, ~{61}], ~[~{62}, s]] with one shared allocation for s."""
    heap = Buffer(2)
    payload = bytes(range(129)) + b"\0"
    fruit_offset = 32
    heap.data.extend(heap.ref(FORK, 4, 0) + heap.ref(INDIRECT, 16, 0))
    heap.data.extend(heap.ref(INDIRECT, 8, 0) + heap.inline_fruit(b"a"))
    heap.data.extend(heap.long_ref(FORK, 12, 8))
    heap.data.extend(heap.ref(FORK, 28, 8) + heap.ref(FRUIT, fruit_offset, 8))
    heap.data.extend(heap.long_ref(FORK, 20, 16))
    heap.data.extend(heap.inline_fruit(b"b") + heap.ref(INDIRECT, 24, 16))
    heap.data.extend(heap.long_ref(INDIRECT, 8, 24))
    heap.data.extend(bytes(2) + heap.inline_fruit(b""))
    heap.data.extend(uleb128(len(payload)) + payload)
    fruit_slot = len(heap.data)
    fruit_long_ref = fruit_slot + heap.width
    heap.data.extend(heap.ref(INDIRECT, fruit_long_ref, 0))
    heap.data.extend(heap.long_ref(FRUIT, fruit_offset, fruit_long_ref))

    left, _ = heap.resolve(0, 0)
    right, _ = heap.resolve(2, 0)
    left_shared, left_path = heap.resolve(left.offset, left.base)
    right_shared, right_path = heap.resolve(right.offset + heap.width, right.base)
    assert left_shared == right_shared == Fork(offset=12, base=8)
    assert left_path == (8,) and right_path == (24, 8)
    assert heap.resolve(left.offset + heap.width, left.base) == (b"a", ())
    assert heap.resolve(right.offset, right.base) == (b"b", ())
    child, _ = heap.resolve(left_shared.offset, left_shared.base)
    assert child == Fork(offset=28, base=8)
    assert heap.resolve(child.offset, child.base) == (None, ())
    assert heap.resolve(child.offset + heap.width, child.base) == (b"", ())
    assert heap.resolve(right_shared.offset + heap.width, right_shared.base) == (payload, ())
    assert heap.resolve(fruit_slot, 0) == (payload, (fruit_long_ref,))

    print("\nIntermingled shared trees (w=2)")
    heap.dump(0, 4, "root fork")
    heap.dump(4, 4, "left tree")
    heap.dump(8, 4, "terminal long ref shared by both paths")
    heap.dump(12, 4, "shared fork, base 0x08")
    heap.dump(16, 4, "terminal long ref to right tree")
    heap.dump(20, 4, "right tree, base 0x10")
    heap.dump(24, 4, "backward hop from 0x18 to 0x08")
    heap.dump(28, 4, "leaf and empty fruit")
    heap.dump(fruit_offset, 6, "130-byte fruit: ULEB128 length and first four bytes")
    heap.dump(fruit_long_ref, 4, "terminal long ref back to the same fruit")


def far_fork():
    """Use an anonymous mapping to exercise a real offset beyond 1 GiB."""
    chain_offsets = (0x20, 0x1000001C, 0x20000018, 0x30000014, 0x40000010)
    terminal = chain_offsets[-1]
    fork_offset = terminal + 4
    with mmap.mmap(-1, fork_offset + 4) as data:
        heap = Buffer(2, data)
        records = [(0, heap.ref(INDIRECT, chain_offsets[0], 0), "ref to the first long ref")]
        for offset, target in zip(chain_offsets, chain_offsets[1:]):
            records.append((offset, heap.long_ref(INDIRECT, target, offset), "forward hop"))
        records.append((terminal, heap.long_ref(FORK, fork_offset, terminal), "terminal fork ref"))
        records.append((fork_offset, bytes(4), "fork ~[~, ~]"))
        for offset, raw, _ in records:
            data[offset : offset + len(raw)] = raw
        fork, chain = heap.resolve(0, 0)
        assert fork == Fork(fork_offset, terminal)
        assert chain == chain_offsets
        assert heap.resolve(fork.offset, fork.base) == (None, ())
        assert heap.resolve(fork.offset + heap.width, fork.base) == (None, ())
        print("\nFork beyond 1 GiB (w=2, sparse anonymous mapping)")
        for offset, raw, label in records:
            heap.dump(offset, len(raw), label)


def check_bad_chain():
    heap = Buffer(2)
    heap.data.extend(heap.ref(INDIRECT, 2, 0))
    heap.data.extend(heap.long_ref(INDIRECT, 2, 2))
    try:
        heap.resolve(0, 0)
    except ValueError as error:
        assert "cycle in long refs" in str(error)
        print("\nIndirect cycle rejected")
    else:
        raise AssertionError("indirect cycle accepted")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--forks", type=int, default=100_000, help="dense spine length")
    parser.add_argument("--skip-far", action="store_true", help="skip the 1 GiB virtual mapping")
    args = parser.parse_args()
    if args.forks < 1:
        parser.error("--forks must be at least one")

    print("Dense spine (packed bytes and actual lookups)")
    for width in (2, 3, 4):
        dense_spine(width, args.forks)
    shared_trees()
    if not args.skip_far:
        far_fork()
    check_bad_chain()


if __name__ == "__main__":
    main()
