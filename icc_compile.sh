#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cmake -S "$project_dir" -B "$project_dir/build/intel" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=icpx "$@"
cmake --build "$project_dir/build/intel" --parallel 4
