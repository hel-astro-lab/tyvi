# Build system

This document contains instructions on how configure and build tyvi.

## Configuration with CMake presets

Tyvi can be configured with cmake presets.
To see available presets (from `CMakePresets.json`):

```shell
> cmake --list-presets
Available configure presets:

  "unixlike-gcc-debug"                        - gcc Debug
  "unixlike-gcc-debug-wout-static-analysis"   - gcc Debug
  "unixlike-gcc-release"                      - gcc Release
  "unixlike-clang-debug"                      - clang Debug
  "unixlike-clang-debug-wout-static-analysis" - clang Debug
  "unixlike-clang-release"                    - clang Release
```

For example gcc debug build directory can be configured with:

```shell
> cmake --preset unixlike-gcc-debug
```

Presets are configured to create build directories to `out/build/`.
See `docs/build-system-options.md` for what CMake options can be defined
in the presets or be give as command line parameters in form `-D<option>=<value>`.

See CMake documentation for how to choose the compiler manually [^CXX][^CCXX].

Users can add their own and extend existing presets with `CMakeUserPresets.json`.
For more information see CMake documentation [^pre].

[^CXX]: https://cmake.org/cmake/help/latest/envvar/CXX.html
[^CCXX]: https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER.html
[^pre]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
