#!/bin/sh

this_script_dir=$(dirname "$(readlink -f "$0")")

python3 "$this_script_dir/sort_includes.py"
cmake --build cmake-build-debug-arm32 --target fmt
