#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import ctypes
import os
import unittest
import subprocess

import api_wrapper

LIB_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build", "libeval-sanitize.so"))
if not os.path.exists(LIB_PATH):
    raise ImportError(
        "No C eval library is found, please build it using ninja\n"
        f"Was searching for {LIB_PATH}")

class TestEval(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        api_wrapper.add_library_search_dirs([os.path.dirname(LIB_PATH)])
        cls.lib: api_wrapper = api_wrapper.load_library("eval-sanitize")

    def test(self):
        state = ctypes.c_void_p()
        try:
            ret = self.lib.eval_load_json("""
                {
                    "cells": "^^**^**",
                    "words": [],
                    "control_stack": [0],
                    "value_stack": []                                  
                }
                """, ctypes.byref(state))
            assert ret == 0
        finally:
            self.lib.eval_free(ctypes.byref(state))

if __name__ == "__main__":
    unittest.main()
