#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cmake -S "$project_dir" -B "$project_dir/build/release" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "$project_dir/build/release" --parallel 4
