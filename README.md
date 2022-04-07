# ThreadScript

ThreadScript is a simple embeddable scripting language intended primarily as
a configuration language for multithreaded C++ programs. It is implemented in
C++20.

## Author

Martin Beran

martin@mber.cz

## Repository structure

- `doc/` – documentation
- `src/` – source code
- `test/` – automatic tests, including various sample scripts

## Build

```
mkdir build
cd build
cmake ..
make -j `nproc`
```

## Run

```
build/src/ts -t `nproc` script.ts
```
