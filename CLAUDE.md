# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Tyvi is a C++23 GPU parallelization library for physics simulation applications, built on HIP and rocThrust with `std::mdspan` as a central abstraction. It provides multi-dimensional grid data structures with asynchronous GPU execution and host-device synchronization.

## Build Commands

```bash
# Configure and build (debug, hipcc compiler)
cmake --preset unixlike-hipcc-debug
cmake --build out/build/unixlike-hipcc-debug

# Run all tests
cd out/build/unixlike-hipcc-debug && ctest

# Run a single test (executables are named <test>.test)
./test/mdgrid.test
./test/sstd.test

# Available presets (list with: cmake --preset list)
# unixlike-hipcc-debug                  - HIP Debug build
# unixlike-hipcc-debug-static-analysis  - HIP Debug + clang-tidy + cppcheck
# unixlike-hipcc-release                - HIP RelWithDebInfo
# unixlike-clang-cpu-debug              - CPU Debug build (no GPU required)
```

### CPU backend (macOS / no GPU)

```bash
# Requires Homebrew LLVM for full C++23 support:
#   brew install llvm

cmake --preset unixlike-clang-cpu-debug
cmake --build out/build/unixlike-clang-cpu-debug
cd out/build/unixlike-clang-cpu-debug && ctest
```

### Requirements

- **HIP backend**: CMake >= 3.21, hipcc (tested with LLVM 18), rocThrust v3.1.0 (rocm 6.2.0)
- **CPU backend**: CMake >= 3.21, Homebrew LLVM (`/opt/homebrew/opt/llvm/bin/clang++`) for C++23 support

## Architecture

### Core Components (`src/tyvi/`)

- **`mdgrid.h`** — Main multi-dimensional grid class. Wraps `mdgrid_buffer` with GPU execution (`for_each`, `for_each_index`), host-device sync (`sync_to_staging`/`sync_from_staging`), and async work DAG (`mdgrid_work`). Provides `TYVI_CAPTURE_SINGLE_MDS` and `TYVI_CMDS` macros for capturing mdspan views in GPU kernels.
- **`mdgrid_buffer.h`** — Low-level buffer management with custom accessor policies for element-level and grid-level mdspan access. Supports `invalidating_resize()`.
- **`mdgrid_work.cpp`** — Async stream management using HIP streams with event-based dependency tracking. `stream_factory` reuses streams via `std::future`/`std::promise`. `when_all()` synchronizes multiple work items.
- **`mdspan.h`** — mdspan utilities: `geometric_extents`, `geometric_mdspan`, `index_space_iterator`, `index_space_view`, and concepts (`layout_mapping`, `static_mds_extents`, etc.).
- **`sstd.h`** — Small standard library extensions: `ipow()` (constexpr integer power), `immovable` base class.

### Dependencies (managed via CPM)

- **rocThrust/rocPRIM** — GPU parallel algorithms (system install at `/opt/rocm`)
- **Kokkos mdspan** — `std::mdspan` reference implementation
- **Boost.ut** — Unit testing framework (only when `BUILD_TESTING=ON`)

## Code Conventions

- **C++23** with heavy use of concepts, ranges, mdspan, constexpr, and structured bindings
- **File extensions**: `.h` for headers, `.c++` for source/test files
- **Formatting**: 100-column limit, 4-space indent, no tabs, Attach brace style (run `clang-format`)
- **Naming**: `snake_case` for variables, functions, and classes; `UPPER_CASE` for template parameters
- **Namespace**: `tyvi`, `tyvi::sstd`, `tyvi::detail`
- **Headers**: `#pragma once`
- **Commit messages**: conventional commits — `<type>(<scope>): <subject>` (e.g., `fix(mdgrid_work): make const correct`)
- **Backend dispatch**: Always use preprocessor directives (`#ifdef TYVI_USE_CPU_BACKEND` / `#elif defined(TYVI_USE_HIP_BACKEND)`) for CPU/HIP backend selection. Never use `if constexpr (backend::is_cpu)` or similar constexpr checks — the HIP branch references names (thrust, HIP runtime) that don't exist on the CPU backend, so `#ifdef` is required.

## Testing

Tests use Boost.ut framework. Each test file `test/test_<name>.c++` compiles to `<name>.test`. Test pattern:

```cpp
#include <boost/ut.hpp>
using namespace boost::ut;

[[maybe_unused]]
const suite<"name"> _ = [] {
    "test_name"_test = [] {
        expect(condition);
    };
};

int main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
```

## SIMD Vectorization Analysis

The CPU backend uses `#pragma omp simd` on the innermost loop of `detail::nested_for` (mdgrid.h:223) to auto-vectorize kernel lambdas. See `runko_simd_analysis.md` in the runko root for the full per-kernel report.

### How to analyze vectorization

```bash
# 1. Configure a release build with clang vectorization diagnostics
cmake -B build-vec-analysis \
  -DCMAKE_CXX_COMPILER=mpic++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DTYVI_BACKEND=cpu \
  -DCMAKE_CXX_FLAGS="-O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"

# 2. Build and capture diagnostics (stderr)
cmake --build build-vec-analysis 2>/tmp/vec_stderr.txt

# 3. Quick summary
grep -c "vectorized loop" /tmp/vec_stderr.txt    # vectorized count
grep -c "not vectorized"  /tmp/vec_stderr.txt    # failed count

# 4. Per-function attribution: compile a single file with
#    -fsave-optimization-record (must omit -flto or yaml won't be generated).
#    Parse the resulting .opt.yaml for Name=Vectorized entries.
#    Use llvm-cxxfilt to demangle the Function field.
mpic++ [flags] -O3 -fsave-optimization-record -c core/emf/yee_lattice_FDTD2.c++ -o /tmp/fdtd2.o
grep -B2 "Name:.*Vectorized" /tmp/fdtd2.opt.yaml | grep "Function:" | llvm-cxxfilt
```

### Key things that block vectorization

- **Mixed float/double**: `1.0` literals are `double`; use `1.0f` when `value_type` is `float` to avoid halving SIMD width
- **Non-vectorizable calls**: `std::sqrt`, `std::min`, `std::max` may not vectorize; use `tyvi::sstd::sqrt/min/max` instead
- **Inner loops**: A loop body containing another loop (e.g., stencil iteration) prevents the outer `#pragma omp simd` from vectorizing
- **Complex accessors**: Deep accessor chains (e.g., `submdspan` with static extents) can prevent the compiler from proving memory safety

## Contributing

Install pre-commit hooks before committing:
```bash
pip install pre-commit
pre-commit install
```
