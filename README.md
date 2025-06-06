Tyvi is C++23 GPU parallelization library which focuses on physics simulation applications.
It is build on top of HIP and rocThrust library with `std::mdspan` playing a central role.

## Requirements

- CMake (>= v3.21)
- C++23 hip compiler (tested with llvm 18)
- rocThrust (tested with rocThrust v3.1.0 from rocm v6.2.0)

## Quick start

Build in debug mode with hipcc and run tests:

```shell
> cmake --preset cmake --preset unixlike-hipcc-debug
> cd out/build/unixlike-hipcc-debug
> make -j && ctest
```

Use `cmake --preset list` to see all available configuration presets.

See `docs/build-system.md` and `docs/build-system-options.md` for more details.

## Contributing

Before commiting, please install `pre-commit` hooks with:

```shell
> pip install pre-commit
> pre-commit install
```

See `docs/pre-commit-hooks.md` for more details.
