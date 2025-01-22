# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import ctypes

import parser  # pylint: disable=wrong-import-order,deprecated-module

NodeData = ctypes.c_size_t


class NodeLib:
    eval_lib: ctypes.CDLL

    def __init__(self, eval_lib):
        self.eval_lib = eval_lib
        self.eval_lib.node_new_tree.argtypes = [
            ctypes.c_size_t, ctypes.c_size_t
        ]
        self.eval_lib.node_new_tree.restype = NodeData
        self.eval_lib.node_new_app.argtypes = [
            ctypes.c_size_t, ctypes.c_size_t
        ]
        self.eval_lib.node_new_app.restype = NodeData
        self.eval_lib.node_new_data.argtypes = [ctypes.c_size_t]
        self.eval_lib.node_new_data.restype = NodeData
        self.eval_lib.node_new_invalid.argtypes = []
        self.eval_lib.node_new_invalid.restype = NodeData

        self.eval_lib.node_tag.argtypes = [NodeData]
        self.eval_lib.node_tag.restype = ctypes.c_size_t
        self.eval_lib.node_lhs.argtypes = [NodeData]
        self.eval_lib.node_lhs.restype = ctypes.c_size_t
        self.eval_lib.node_rhs.argtypes = [NodeData]
        self.eval_lib.node_rhs.restype = ctypes.c_size_t
        self.eval_lib.node_data.argtypes = [NodeData]
        self.eval_lib.node_data.restype = ctypes.c_size_t

        self.eval_lib.node_tag_tree.argtypes = []
        self.eval_lib.node_tag_tree.restype = ctypes.c_size_t
        self.eval_lib.node_tag_app.argtypes = []
        self.eval_lib.node_tag_app.restype = ctypes.c_size_t
        self.eval_lib.node_tag_data.argtypes = []
        self.eval_lib.node_tag_data.restype = ctypes.c_size_t

    def new_tree(self, lhs=None, rhs=None) -> NodeData:
        lhs = lhs or self.new_invalid()
        rhs = rhs or self.new_invalid()
        return self.eval_lib.node_new_tree(lhs, rhs)

    def new_app(self, lhs, rhs) -> NodeData:
        return self.eval_lib.node_new_app(lhs, rhs)

    def new_data(self, data) -> NodeData:
        return self.eval_lib.node_new_data(data)

    def new_invalid(self) -> NodeData:
        return self.eval_lib.node_new_invalid()

    def tag(self, node: NodeData) -> int:
        return self.eval_lib.node_tag(node)

    def lhs(self, node: NodeData) -> int:
        return self.eval_lib.node_lhs(node)

    def rhs(self, node: NodeData) -> int:
        return self.eval_lib.node_rhs(node)

    def data(self, node: NodeData) -> int:
        return self.eval_lib.node_data(node)

    def tag_tree(self) -> int:
        return self.eval_lib.node_tag_tree()

    def tag_app(self) -> int:
        return self.eval_lib.node_tag_app()

    def tag_data(self) -> int:
        return self.eval_lib.node_tag_data()


def encode_pure_tree(root: parser.Node | None,
                     eval_lib: ctypes.CDLL) -> (int, [NodeLib]):
    node_lib = NodeLib(eval_lib)
    nodes: list[int] = []
    if root is None:
        return 0, nodes

    def aux(n: parser.Node) -> int:
        nonlocal nodes
        tree_or_app = isinstance(n, (parser.TreeNode, parser.Application))
        assert tree_or_app, (
            "Encoding to binary nodes only supported for tree nodes\n"
            f"but found {type(n)}")

        if isinstance(n, parser.Application):
            create_node = node_lib.new_app
        else:
            create_node = node_lib.new_tree
        nodes.append(0)
        i = len(nodes) - 1

        match len(n.children):
            case 0:
                nodes[i] = create_node(node_lib.new_invalid(),
                                       node_lib.new_invalid())
                return i
            case 1:
                lhs_i = aux(n.children[0])
                nodes[i] = create_node(lhs_i - i, node_lib.new_invalid())
                return i
            case 2:
                lhs_i = aux(n.children[0])
                rhs_i = aux(n.children[1])
                nodes[i] = create_node(lhs_i - i, rhs_i - i)
                return i
            case _:
                assert False, "Unreachable"

    root_node = aux(root)
    return (root_node, nodes)


# NOTE: better use this for very small numbers (< 255), To be able to represent a byte
# Then, we can use a list to represent true list of bytes in tree-calculus
def value_as_list_to_number(eval_lib, root: int, tree: list[int]) -> int:
    """
    Do conversion with the following equations
    ^ = 0 as base, ^(base) = base + 1
    Example: ^(^(^(^))) = ^(^(^^)) = 3
    Basically, a left-directed linked list of ^
    Function returns a bytearray to be interpret further
    """

    node_lib = NodeLib(eval_lib)

    cur = root
    i = 0
    while True:
        node = tree[cur]
        lhs = node_lib.lhs(node)
        rhs = node_lib.rhs(node)
        if lhs == node_lib.new_invalid() and rhs == node_lib.new_invalid():
            break
        assert rhs == node_lib.new_invalid(), (
            f"Value is expected to be a linked list, encountered a node {node}"
        )
        i += 1
        cur += lhs

    return i


def value_as_tree_to_number(eval_lib, root: int, tree: list[int]) -> int:
    """
    This encoding is much more efficient in terms of space, we define:
    0 = ^
    1 = ^^
    2 = ^^^
    3 = ^(^^^)
    and so on...
    Basically, we exploit node saturation to maximum
    """
    pass


def dump_tree(eval_lib, root: int, tree: list[int]) -> str:
    node_lib = NodeLib(eval_lib)
    invalid = node_lib.new_invalid()
    lines = []

    def add_line(index, node_mark, prefix, new_prefix):
        n = tree[index]
        line = [prefix, f'{node_mark}[{bin(n)}]\n']
        lines.append(''.join(line))
        lhs = node_lib.lhs(n)
        rhs = node_lib.rhs(n)
        is_last = rhs == invalid
        if lhs != invalid:
            aux(index + lhs, new_prefix, is_last)
        is_last = True
        if rhs != invalid:
            aux(index + rhs, new_prefix, is_last)

    def aux(index, prefix='', is_last=False):
        nonlocal lines
        line_prefix = prefix + ('└── ' if is_last else '├── ')
        new_prefix = prefix + ('    ' if is_last else '│   ')

        n = tree[index]
        tag = node_lib.tag(n)
        if tag == node_lib.tag_app():
            add_line(index, '$', line_prefix, new_prefix)
        elif tag == node_lib.tag_tree():
            add_line(index, '^', line_prefix, new_prefix)
        else:
            raise RuntimeError("Unreachable")

    aux(root)
    return ''.join(lines)
