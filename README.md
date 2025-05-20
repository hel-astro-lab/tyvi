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