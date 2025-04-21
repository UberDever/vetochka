#!/bin/bash

# to generate api wrapper
# ctypesgen api.h -o api_wrapper.py

SCRIPT_DIR=$(dirname $(realpath ${0}))

# suppress python3 leaks
export LSAN_OPTIONS="suppressions=${SCRIPT_DIR}/../eval/asan.supp"
asan_path=$(realpath $(clang -print-file-name=libasan.so))
ubsan_path=$(realpath $(clang -print-file-name=libubsan.so))
preload="${asan_path}:${ubsan_path}"
LD_PRELOAD=$preload python3 ${SCRIPT_DIR}/evaltest.py
