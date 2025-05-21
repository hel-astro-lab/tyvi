Under construction...

## Build system features

By default (collectively known as `ENABLE_DEVELOPER_MODE`):

 * Address Sanitizer and Undefined Behavior Sanitizer
 * Warnings as errors
 * clang-tidy and cppcheck static analysis
 * CPM for dependencies

## Building

```shell
> cmake -Bbuild .
> ninja -C build
```

### CMake presets

CMake presets require CMake >= v21.0.0 (see commit `2fd7364`).

CMake presets are set of common build options for different usecases.
They can be specified during build directory configuratio with `--preset <configure-preset>`.
Global presets are defined in `CMakePresets.json`.
Users can define their own presets in `CMakeUserPresets.json`.

Example:

```shell
> cmake --preset test-unixlike-gcc-debug -Bbuild .
> ninja -C build
```