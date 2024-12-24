# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import parser  # pylint: disable=wrong-import-order,deprecated-module


# NOTE: This is not for python, but for system language like C
# NOTE: This can be used with arbitrary-length integers, since
# we traverse nodes by their offset in the memory (or array)
# but this encoding is simpler
class Node:
    repr: int

    int_size = 31
    int_max = 2**int_size - 1
    LEAF = 0
    STEM = 1
    FORK = 2
    APP = 3

    def __init__(self, tag=0, lhs=0, rhs=0):
        self.repr = 0
        self.set_tag(tag)
        self.set_lhs(lhs)
        self.set_rhs(rhs)

    def tag(self):
        return (self.repr & 0b11 << (self.int_size * 2)) >> (self.int_size * 2)

    def lhs(self):
        return (self.repr & self.int_max << self.int_size) >> self.int_size

    def rhs(self):
        return self.repr & self.int_max

    def set_tag(self, tag):
        assert tag <= self.APP
        self.repr = self.repr | tag << (self.int_size * 2)

    def set_lhs(self, n):
        assert n <= self.int_max
        self.repr = self.repr | (n << self.int_size)

    def set_rhs(self, n):
        assert n <= self.int_max
        self.repr = self.repr | n

    def __eq__(self, obj):
        return self.repr == obj.repr

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f'Node({self.tag()}, {self.lhs()}, {self.rhs()})'


def encode_tree_nodes(root: parser.Node) -> (int, [Node]):

    nodes = []

    def aux(n: parser.Node) -> int:
        nonlocal nodes
        tree_or_app = isinstance(n, (parser.TreeNode, parser.Application))
        assert tree_or_app, "Encoding to binary nodes \
            only supported for tree nodes"

        tag = None
        if isinstance(n, parser.Application):
            tag = Node.APP
        else:
            match len(n.children):
                case 0:
                    tag = Node.LEAF
                case 1:
                    tag = Node.STEM
                case 2:
                    tag = Node.FORK
                case _:
                    assert False, "Unreachable"
        node = Node()
        node.set_tag(tag)
        nodes.append(node)
        inserted_at = len(nodes) - 1

        match len(n.children):
            case 0:
                return inserted_at
            case 1:
                inserted_at_lhs = aux(n.children[0])
                nodes[inserted_at].set_lhs(inserted_at_lhs - inserted_at)
                return inserted_at
            case 2:
                inserted_at_lhs = aux(n.children[0])
                nodes[inserted_at].set_lhs(inserted_at_lhs - inserted_at)
                inserted_at_rhs = aux(n.children[1])
                nodes[inserted_at].set_rhs(inserted_at_rhs - inserted_at)
                return inserted_at
            case _:
                assert False, "Unreachable"

    root_node = aux(root)
    return (root_node, nodes)
