#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import subprocess
import sys
import os
import shutil

BUILD_DIR = os.path.join("..", "build")
TARGET = os.path.join(BUILD_DIR, "evaluate")

CC = "clang"
CFLAGS = "--std=c99 -Wall -O3 -I."
SOURCES = ["eval.c", "node.c"]


def compile_c_linux(source, destination):
    return [CC, *CFLAGS.split(), "-c", source, "-o", destination]


def link_c_linux(objects, destination):
    return [CC, "-shared", "-fPIC", "-o", destination, *objects]


def compile_c(source, destination):
    if sys.platform == "win32":
        assert False, "Windows bruh"
    else:
        return compile_c_linux(source, destination)


def link_c(objects, destination):
    if sys.platform == "win32":
        assert False, "Windows bruh"
    else:
        return link_c_linux(objects, destination)


def new_clean_dir(path):
    if not os.path.exists(path):
        print(f"$> mkdir {path}")
        os.mkdir(path)
    else:
        print(f"$> rm -rf {path}")
        shutil.rmtree(path)
        print(f"$> mkdir {path}")
        os.mkdir(path)


def build():
    new_clean_dir(BUILD_DIR)
    compiled_obj = [os.path.join(BUILD_DIR, s + ".o") for s in SOURCES]
    commands = [compile_c(s, o) for s, o in zip(SOURCES, compiled_obj)]
    commands.append(link_c(compiled_obj, TARGET))
    for cmd in commands:
        print(f"""$> {" ".join(cmd)}""")
        subprocess.run(cmd, check=True)


if __name__ == "__main__":
    build()
