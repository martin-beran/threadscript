# ThreadScript

ThreadScript is a simple embeddable scripting language intended primarily as
a configuration language for multithreaded C++ programs. It is implemented in
C++20.

## Author

Martin Beran

<martin@mber.cz>

## License

This software is available under the terms of BSD 2-Clause License, see
file [LICENSE.md](LICENSE.html).

## Repository structure

- `doc/` – documentation
- `src/` – source code
- `test/` – automatic tests, including various sample scripts

## Build

Build libraries, the command line interpreter, tests, and basic (Markdown)
documentation:

    mkdir build
    cd build
    cmake ..
    cmake --build . -j `nproc`

A clean build of libraries, the command line interpreter, tests, all
documentation (Markdown and Doxygen), running `clang-tidy` on sources, and
running tests:

    mkdir build
    cd build
    cmake ..
    cmake --build . -t full

The full build with tests like the previous one, but repeated with all
supported sanitizers:

    rm -rf build-sanitizer-*
    ./make-everyting.sh

## Run

    build/src/ts -t `nproc` script.ts

## [Documentation](html/index.html)

Documentation of ThreadScript is generated by Doxygen from comments in source
code and from files in subdirectory `doc/` into build subdirectory
[html/](html/index.html) by command

    make doxygen
