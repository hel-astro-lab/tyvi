#pragma once

#include <cmath>
#include <concepts>
#include <ranges>
#include <stdexcept>
#include <type_traits>

#include "tyvi/backend.h"

#ifdef TYVI_USE_HIP_BACKEND
#include "thrust/memory.h"
#include "hip/hip_runtime.h"
#endif

namespace tyvi::sstd {

template<std::integral T>
[[nodiscard]]
constexpr T
ipow(const T base, const T exponent) {
    if (exponent < T{ 0 }) {
        throw std::logic_error{ "Integers can not be raised to negative power." };
    }
    auto result = T{ 1 };
    for (auto _ : std::views::iota(T{ 0 }, exponent)) {
        result *= base; // cppcheck-suppress useStlAlgorithm
    }
    return result;
}

struct immovable {
    constexpr immovable()           = default;
    constexpr ~immovable() noexcept = default;

    constexpr immovable(immovable&&) noexcept                  = delete; // move constructor
    constexpr immovable& operator=(immovable&& other) noexcept = delete; // move assignment

    constexpr immovable(const immovable&)            = delete; // copy constructor
    constexpr immovable& operator=(const immovable&) = delete; // copy assignment
};

// =====================================================================
// raw_ref — identity on CPU, thrust::raw_reference_cast on HIP
// =====================================================================

template<typename T>
constexpr auto&
raw_ref(T& x) {
#ifdef TYVI_USE_CPU_BACKEND
    return x;
#elif defined(TYVI_USE_HIP_BACKEND)
    return thrust::raw_reference_cast(x);
#endif
}

// =====================================================================
// atomic_add — direct += on CPU, unsafeAtomicAdd on HIP
// =====================================================================

template<typename T>
constexpr void
atomic_add(T* addr, T val) {
#ifdef TYVI_USE_CPU_BACKEND
    *addr += val;
#elif defined(TYVI_USE_HIP_BACKEND)
    ::unsafeAtomicAdd(addr, val);
#endif
}

// =====================================================================
// sin / cos — constexpr wrappers for CPU and HIP
// =====================================================================

template<typename T>
constexpr auto
sin(const T x) {
    if constexpr (std::is_same_v<T, float>) {
        return ::sinf(x);
    } else {
        return ::sin(x);
    }
}

template<typename T>
constexpr auto
cos(const T x) {
    if constexpr (std::is_same_v<T, float>) {
        return ::cosf(x);
    } else {
        return ::cos(x);
    }
}

// =====================================================================
// sqrt — constexpr wrapper for CPU and HIP
// =====================================================================

template<typename T>
constexpr auto
sqrt(const T x) {
    if constexpr (std::is_same_v<T, float>) {
        return ::sqrtf(x);
    } else {
        return ::sqrt(x);
    }
}

// =====================================================================
// min / max — SIMD-friendly value-based min/max
// =====================================================================
// Using ternary instead of std::min/std::max to avoid reference
// semantics that can inhibit auto-vectorization.

template<typename T>
constexpr auto
min(const T a, const T b) {
    return a < b ? a : b;
}

template<typename T>
constexpr auto
max(const T a, const T b) {
    return a > b ? a : b;
}

// =====================================================================
// abs — SIMD-friendly absolute value
// =====================================================================

template<typename T>
constexpr auto
abs(const T x) {
    return x < T{ 0 } ? -x : x;
}

// =====================================================================
// fmod — constexpr wrapper for CPU and HIP
// =====================================================================

template<typename T>
constexpr auto
fmod(const T a, const T b) {
    if constexpr (std::is_same_v<T, float>) {
        return ::fmodf(a, b);
    } else {
        return ::fmod(a, b);
    }
}

// =====================================================================
// floor — SIMD-friendly floor (FRINTM on ARM NEON)
// =====================================================================

template<typename T>
constexpr auto
floor(const T x) {
    if constexpr (std::is_same_v<T, float>) {
        return ::floorf(x);
    } else {
        return ::floor(x);
    }
}

} // namespace tyvi::sstd
