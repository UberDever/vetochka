#!/bin/bash

# to generate api wrapper
# ctypesgen api.h -o api_wrapper.py

SCRIPT_DIR=$(dirname $(realpath ${0}))

# suppress python3 leaks
export LSAN_OPTIONS="suppressions=${SCRIPT_DIR}/../eval/asan.supp"
LD_PRELOAD=$(clang -print-file-name=libasan.so):$(clang -print-file-name=libubsan.so) python3 ${SCRIPT_DIR}/evaltest.py
