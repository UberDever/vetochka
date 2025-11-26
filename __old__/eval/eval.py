# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=too-few-public-methods

import os
import ctypes

LIB_PATH = os.path.join(os.path.dirname(__file__), "..", "build",
                        "libeval-release.so")
if not os.path.exists(LIB_PATH):
    raise ImportError(
        "No C eval library is found, please build it using ninja\n"
        f"Was searching for {LIB_PATH}")

EvalState = ctypes.c_void_p


def load_rt_lib():
    return ctypes.CDLL(LIB_PATH)


class EvalLib:

    def __init__(self, rt_lib):
        self.rt_lib = rt_lib

        self.rt_lib.eval_init.argtypes = [
            ctypes.POINTER(EvalState), ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t), ctypes.c_size_t
        ]
        self.rt_lib.eval_init.restype = ctypes.c_size_t
        self.rt_lib.eval_free.argtypes = [ctypes.POINTER(EvalState)]
        self.rt_lib.eval_free.restype = ctypes.c_size_t
        self.rt_lib.eval_eval.argtypes = [EvalState]
        self.rt_lib.eval_eval.restype = ctypes.c_size_t
        self.rt_lib.eval_step.argtypes = [EvalState]
        self.rt_lib.eval_step.restype = ctypes.c_size_t
        self.rt_lib.eval_get_error.argtypes = [
            EvalState,
            ctypes.POINTER(ctypes.c_size_t),
            ctypes.POINTER(ctypes.c_char_p)
        ]
        self.rt_lib.eval_get_error.restype = ctypes.c_size_t

    def init(self, root: int, tree: list[int]) -> EvalState:
        state = EvalState()
        tree_arr = (ctypes.c_size_t * len(tree))(*tree)
        self.rt_lib.eval_init(ctypes.byref(state), root, tree_arr, len(tree))
        return state

    def free(self, state: EvalState) -> EvalState:
        self.rt_lib.eval_free(ctypes.byref(state))
        return state

    def evaluate(self, state: EvalState):
        return self.rt_lib.eval_eval(state)


class Evaluator:
    state: EvalState
    eval_lib: EvalLib

    def __init__(self, rt_lib: ctypes.CDLL, root: int, tree: list[int]):
        self.eval_lib = EvalLib(rt_lib)
        self.state = self.eval_lib.init(root, tree)

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.state = self.eval_lib.free(self.state)

    def evaluate(self):
        self.eval_lib.evaluate(self.state)

    def get_error(self) -> str | None:
        return None
        # if self.state.error_code == 0:
        #     return None
        # return (f'Code: {self.state.error_code}\n'
        #         f'Error: {self.state.error.decode("utf-8")}')
