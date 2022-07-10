#!/bin/sh

set -e

for s in OFF address leak thread undefined; do
    dir="build-sanitizer-$s"
    echo
    echo === BEGIN Build with sanitizer $s ===
    echo
    mkdir -p "$dir"
    (
        cd "$dir"
        cmake -D SANITIZER="$s" "$@" ..
        if [ "$s" = OFF ]; then
            cmake --build . -t full
        else
            cmake --build . -j `nproc`
            cmake --build . -t test -j `nproc`
        fi
    )
    echo
    echo === END Build with sanitizer $s ===
    echo
done
