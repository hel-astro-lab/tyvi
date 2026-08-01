Tyvi is C++23 GPU parallelization library which focuses on physics simulation applications.
It is build on top of HIP and rocThrust library with `std::mdspan` playing a central role.

In addition to HIP, tyvi also supports CPU backend based on the rocThrust OpenMP backend.

## Requirements

- CMake (>= v3.21)
- C++23 compiler (tested with llvm 18 and gcc 14.3.0)
  - Has to support HIP if using hip backend.
  - Has to support OpenMP if using cpu backend.
- rocThrust (tested with rocThrust v3.1.0 from rocm v6.2.0)
  - See `tyvi_BACKEND` in `docs/build-system-options.md` for instructions
    on how to provide the CPU enabled rocThrust to tyvi.
- pika runtime library and its transient dependencies:
  - mpi (eg. openmpi)
  - Boost >=1.77.0
  - hwloc >=1.11.5
  - fmt >=9.0.0
  - spdlog

These should be widely available on different package managers.
In case of configuration errors, consult cmake documentation on
how to specify the library locations.

## Quick start

Build using hip backend in debug mode and run tests:

```shell
> cmake --preset hip-debug
> cd out/build/hip-debug
> make -j 8 && ctest
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

# Acknowledgments

Development of tyvi is supported by an ERC grant (ILLUMINATOR, 101114623).
<img align="center" src="https://cdn.jsdelivr.net/gh/natj/natj.github.io@master/images/erc_logo.png">
