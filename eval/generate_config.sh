#!/usr/bin/env sh
# configure.sh — run once before invoking ninja
echo "Generating config.h"
echo "#define PROJECT_ROOT \"${PWD}\"" > config.h
echo "#define PATH_SEP \"/\"" >> config.h
