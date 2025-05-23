Under construction...

## Quick start

To build and run tests with gcc in debug build:

```shell
> cmake --preset unixlike-gcc-debug
> cd out/build/unixlike-gcc-debug/
> make -j
> ctest
```

See `docs/build-system.md` and `docs/build-system-options.md` for more details.

Before commiting, please install `pre-commit` hooks with:

```shell
> pip install pre-commit
> pre-commit install
```

See `docs/pre-commit-hooks.md` for more details.
