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


def encode_tree_nodes(root: parser.Node | None,
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
