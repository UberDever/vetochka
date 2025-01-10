# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=too-few-public-methods

import os
import ctypes

from . import make_eval

LIB_PATH = os.path.join(os.path.dirname(__file__), make_eval.TARGET)
if not os.path.exists(LIB_PATH):
    raise ImportError(
        "No C eval library is found, please build it using make_eval\n"
        f"Was searching for {LIB_PATH}")


class EvalState(ctypes.Structure):
    _fields_ = [
        ('root', ctypes.c_uint64),
        ('nodes', ctypes.POINTER(ctypes.c_uint64)),
        ('nodes_size', ctypes.c_uint64),
        ('error_code', ctypes.c_int8),
        ('error', ctypes.c_char_p),
    ]


class Evaluator:
    state: EvalState
    eval_lib: ctypes.CDLL

    def __init__(self):
        self.eval_lib = ctypes.CDLL(LIB_PATH)
        self.state = EvalState()
        self.state.root = 0
        self.state.nodes = None
        self.state.nodes_size = 0
        self.state.error_code = 0
        self.state.error = None

        self.eval_lib.reset.argtypes = [ctypes.POINTER(EvalState)]
        self.eval_lib.reset.restype = None
        self.eval_lib.eval.argtypes = [ctypes.POINTER(EvalState)]
        self.eval_lib.eval.restype = None

        self.reset()

    def set_tree(self, root: int, tree: list[int]):
        self.reset()
        self.state.root = root
        self.state.nodes = (ctypes.c_uint64 * len(tree))(*tree)

    def evaluate(self):
        self.eval_lib.eval(ctypes.byref(self.state))

    def reset(self):
        self.eval_lib.reset(ctypes.byref(self.state))

    def get_error(self) -> str | None:
        if self.state.error_code == 0:
            return None
        return (f'Code: {self.state.error_code}\n'
                f'Error: {self.state.error.decode("utf-8")}')
