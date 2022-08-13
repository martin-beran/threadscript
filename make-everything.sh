#!/bin/sh

set -e

: ${SANITIZERS:=OFF address leak thread undefined}

for s in $SANITIZERS; do
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
