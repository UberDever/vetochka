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
        ('root', ctypes.c_size_t),
        ('nodes', ctypes.POINTER(ctypes.c_size_t)),
        ('nodes_size', ctypes.c_size_t),
        ('stack', ctypes.POINTER(ctypes.c_size_t)),
        ('error_code', ctypes.c_int8),
        ('error', ctypes.c_char_p),
    ]


def load_eval_lib():
    return ctypes.CDLL(LIB_PATH)


class Evaluator:
    state: EvalState
    eval_lib: ctypes.CDLL

    def __init__(self, eval_lib):
        self.eval_lib = eval_lib
        self.state = EvalState()
        self.state.root = 0
        self.state.nodes = None
        self.state.nodes_size = 0
        self.state.stack = None
        self.state.error_code = 0
        self.state.error = None

        self.eval_lib.reset.argtypes = [ctypes.POINTER(EvalState)]
        self.eval_lib.reset.restype = None
        self.eval_lib.init.argtypes = [
            ctypes.POINTER(EvalState), ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t), ctypes.c_size_t
        ]
        self.eval_lib.init.restype = None
        self.eval_lib.eval.argtypes = [ctypes.POINTER(EvalState)]
        self.eval_lib.eval.restype = ctypes.c_size_t
        self.eval_lib.step.argtypes = [ctypes.POINTER(EvalState)]
        self.eval_lib.step.restype = ctypes.c_size_t

        self.reset()

    def set_tree(self, root: int, tree: list[int]):
        tree_arr = (ctypes.c_size_t * len(tree))(*tree)
        self.eval_lib.init(ctypes.byref(self.state), root, tree_arr, len(tree))

    def evaluate(self):
        return self.eval_lib.eval(ctypes.byref(self.state))

    def reset(self):
        self.eval_lib.reset(ctypes.byref(self.state))

    def get_error(self) -> str | None:
        if self.state.error_code == 0:
            return None
        return (f'Code: {self.state.error_code}\n'
                f'Error: {self.state.error.decode("utf-8")}')
