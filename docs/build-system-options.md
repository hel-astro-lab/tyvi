# Build system options

Tyvi is uses CMake as build system,
which has been configured to enable easy use of useful developer tools.
This document contains instructions on how to use these tools.

Some of the features are configured with CMake options.
Default values for the options might depend if tyvi is build as toplevel project
or if it is build as subproject for another CMake project.
Subproject defaults can be forced with CMake option: `-Dtyvi_PACKAGING_MAINTAINER_MODE=ON`

### Option defaults

In the options values denote the default values in toplevel mode.
If the default differs in subproject (or packaging maintainer) mode,
then default contains two values seperated with slash (`/`).
The first is toplevel build default and the second is subproject default.

For example:

```
tyvi_ENABLE_FOO:BOOL=ON
tyvi_ENABLE_BAR:BOOL=ON/OFF
```

`tyvi_ENABLE_FOO` defaults to `ON` in both cases.
`tyvi_ENABLE_BAR` defaults to `ON` in toplevel build and to `OFF` in subproject mode.

Lastly postfix `*` for `ON` denotes that it is on if the build system can detect
if the option is supported by the used tooling and all required tools are found.

## Sanitizers

```
tyvi_ENABLE_SANITIZER_ADDRESS:BOOL=OFF
tyvi_ENABLE_SANITIZER_UNDEFINED:BOOL=OFF
tyvi_ENABLE_SANITIZER_LEAK:BOOL=OFF
tyvi_ENABLE_SANITIZER_THREAD:BOOL=OFF
tyvi_ENABLE_SANITIZER_MEMORY:BOOL=OFF
```

Some sanitizers are incompatible with each other.
`cmake/Sanitizers.cmake` contains logic to detect and warn about this.

## Static analysis

```
tyvi_ENABLE_CLANG_TIDY:BOOL=OFF
tyvi_ENABLE_CPPCHECK:BOOL=OFF
tyvi_WARNINGS_AS_ERRORS:BOOL=ON/OFF
```

If enabled clang-tidy [^tidy] and cppcheck [^cppcheck] are run alongside the build process.
Configuration file `.clang-tidy` enables all analysis passes
and removes passes that are not relevant.

See `cmake/CompilerWarnings.cmake` for list of what compiler warnings are used.

[^tidy]: https://clang.llvm.org/extra/clang-tidy/
[^cppcheck]: https://cppcheck.sourceforge.io/

## Hardening

Hardening can help catch bugs during testing.
For example hardened `std::vector::operator[]` detects out of bound accesses during runtime.

```
tyvi_ENABLE_HARDENING:BOOL=OFF
tyvi_ENABLE_GLOBAL_HARDENING:BOOL=OFF
```

Global hardening enables hardening for all the dependencies as well.

## IPO (Interprocedural Optimizatios)

```
tyvi_ENABLE_IPO:BOOL=ON/OFF
```

Enables IPO/LTO.

Static libraries + LTO enables better performance and detection of ODR violations.

## Other options

These options are less used but might come handly at some point.

### Cache

```
tyvi_ENABLE_CACHE:BOOL=*ON/OFF
```

Enables Ccache (compiler cache) [^ccache].

[^ccache]: https://ccache.dev/

### Coverage

```
tyvi_ENABLE_COVERAGE:BOOL=OFF
```

Enables gcov [^gcov] coverage reporting for build binaries.

[^gcov]: https://gcc.gnu.org/onlinedocs/gcc/Gcov.html

### Unity builds

```
tyvi_ENABLE_UNITY_BUILD:BOOL=OFF
```

Enables unity builds.

### User linker

```
tyvi_ENABLE_USER_LINKER:BOOL=OFF
USER_LINKER_OPTION:STRING="<linker>"
```

If enabled CMake option `-DUSER_LINKER_OPTION="<linker>"` can be used to choose linker.
Supported linkers are: `lld`, `gold`, `bfd` and `mold`.

Mold is the most relevant as modern and parallelized (i.e. fastest) linker.

[^lld]: https://lld.llvm.org/
[^gold]: https://www.gnu.org/software/binutils/
[^bfd]: https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_5.html
[^mold]: https://github.com/rui314/mold
