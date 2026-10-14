#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$project_dir/build/release"
exec xboard -xponder -debug -fcp "./morphy" -scp "./morphy" -sgf "$project_dir/games.pgn" -size medium "$@"
