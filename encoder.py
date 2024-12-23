# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import parser  # pylint: disable=wrong-import-order,deprecated-module


# NOTE: This is not for python, but for system language like C
class Node:
    repr: int

    int_size = 31
    int_max = 2**int_size - 1
    LEAF = 0
    STEM = 1
    FORK = 2
    APP = 3

    def __init__(self):
        self.repr = 0

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


def encode_tree_nodes(root: parser.Node) -> [Node]:
    _ = root
