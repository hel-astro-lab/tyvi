#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include "tyvi/backend.h"

#ifdef TYVI_USE_CPU_BACKEND
struct cpu_exec_policy;
#endif

#ifdef TYVI_USE_HIP_BACKEND
#include "thrust/copy.h"
#include "thrust/execution_policy.h"
#include "thrust/for_each.h"
#include "thrust/reduce.h"
#include "thrust/sort.h"
#include "thrust/unique.h"
#endif

namespace tyvi::algo {

// =====================================================================
// Execution policy concept
// =====================================================================

template<typename T>
concept exec_policy =
#ifdef TYVI_USE_CPU_BACKEND
    std::same_as<std::remove_cvref_t<T>, cpu_exec_policy>
#elif defined(TYVI_USE_HIP_BACKEND)
    std::same_as<std::remove_cvref_t<T>, thrust::hip_rocprim::execute_on_stream_nosync>
#endif
;

// =====================================================================
// for_each
// =====================================================================

template<typename InputIt, typename F>
void
for_each(InputIt first, InputIt last, F f) {
    if constexpr (backend::is_cpu) {
        std::for_each(first, last, f);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        thrust::for_each(thrust::device, first, last, f);
    }
#endif
}

template<exec_policy ExecPolicy, typename InputIt, typename F>
void
for_each([[maybe_unused]] ExecPolicy&& policy, InputIt first, InputIt last, F f) {
    if constexpr (backend::is_cpu) {
        std::for_each(first, last, std::move(f));
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        thrust::for_each(std::forward<ExecPolicy>(policy), first, last, std::move(f));
    }
#endif
}

// =====================================================================
// copy
// =====================================================================

template<typename InputIt, typename OutputIt>
OutputIt
copy(InputIt first, InputIt last, OutputIt d_first) {
    if constexpr (backend::is_cpu) {
        return std::copy(first, last, d_first);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::copy(first, last, d_first);
    }
#endif
}

// =====================================================================
// sort
// =====================================================================

template<typename RandomIt>
void
sort(RandomIt first, RandomIt last) {
    if constexpr (backend::is_cpu) {
        std::sort(first, last);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        thrust::sort(first, last);
    }
#endif
}

// =====================================================================
// find
// =====================================================================

template<typename InputIt, typename T>
InputIt
find(InputIt first, InputIt last, const T& value) {
    if constexpr (backend::is_cpu) {
        return std::find(first, last, value);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::find(first, last, value);
    }
#endif
}

// =====================================================================
// sort_by_key
// =====================================================================

template<typename KeyIt, typename ValueIt>
void
sort_by_key(KeyIt keys_first, KeyIt keys_last, ValueIt values_first) {
    if constexpr (backend::is_cpu) {
        const auto n = std::distance(keys_first, keys_last);
        auto indices = std::vector<std::size_t>(static_cast<std::size_t>(n));
        std::iota(indices.begin(), indices.end(), std::size_t{ 0 });
        std::sort(indices.begin(), indices.end(),
            [&](auto a, auto b) { return keys_first[a] < keys_first[b]; });

        using key_t = std::iter_value_t<KeyIt>;
        using val_t = std::iter_value_t<ValueIt>;
        auto sorted_keys = std::vector<key_t>(static_cast<std::size_t>(n));
        auto sorted_vals = std::vector<val_t>(static_cast<std::size_t>(n));
        for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
            sorted_keys[i] = keys_first[indices[i]];
            sorted_vals[i] = values_first[indices[i]];
        }
        std::copy(sorted_keys.begin(), sorted_keys.end(), keys_first);
        std::copy(sorted_vals.begin(), sorted_vals.end(), values_first);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        thrust::sort_by_key(thrust::device, keys_first, keys_last, values_first);
    }
#endif
}

// =====================================================================
// reduce
// =====================================================================

template<typename InputIt, typename T = std::iter_value_t<InputIt>>
T
reduce(InputIt first, InputIt last, T init = T{}) {
    if constexpr (backend::is_cpu) {
        return std::reduce(first, last, init);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::reduce(thrust::device, first, last, init);
    }
#endif
}

template<exec_policy ExecPolicy, typename InputIt, typename T = std::iter_value_t<InputIt>>
T
reduce([[maybe_unused]] ExecPolicy&& policy, InputIt first, InputIt last, T init = T{}) {
    if constexpr (backend::is_cpu) {
        return std::reduce(first, last, init);
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::reduce(std::forward<ExecPolicy>(policy), first, last, init);
    }
#endif
}

// =====================================================================
// reduce_by_key
// =====================================================================

template<typename KeyIn, typename ValIn, typename KeyOut, typename ValOut,
         typename KeyEq = std::equal_to<>, typename ValOp = std::plus<>>
std::pair<KeyOut, ValOut>
reduce_by_key(KeyIn keys_first, KeyIn keys_last, ValIn vals_first,
              KeyOut keys_out, ValOut vals_out,
              KeyEq key_eq = {}, ValOp val_op = {}) {
    if constexpr (backend::is_cpu) {
        if (keys_first == keys_last) { return { keys_out, vals_out }; }

        auto current_key = *keys_first;
        auto current_val = *vals_first;
        ++keys_first;
        ++vals_first;

        while (keys_first != keys_last) {
            if (key_eq(*keys_first, current_key)) {
                current_val = val_op(current_val, *vals_first);
            } else {
                *keys_out++ = current_key;
                *vals_out++ = current_val;
                current_key = *keys_first;
                current_val = *vals_first;
            }
            ++keys_first;
            ++vals_first;
        }
        *keys_out++ = current_key;
        *vals_out++ = current_val;
        return { keys_out, vals_out };
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::reduce_by_key(thrust::device,
            keys_first, keys_last, vals_first,
            keys_out, vals_out, key_eq, val_op);
    }
#endif
}

// =====================================================================
// unique_by_key
// =====================================================================

template<typename KeyIt, typename ValIt>
std::pair<KeyIt, ValIt>
unique_by_key(KeyIt keys_first, KeyIt keys_last, ValIt vals_first) {
    if constexpr (backend::is_cpu) {
        if (keys_first == keys_last) { return { keys_first, vals_first }; }

        auto key_out  = keys_first;
        auto val_out  = vals_first;
        auto prev_key = *keys_first;
        *key_out      = prev_key;
        *val_out      = *vals_first;
        ++keys_first;
        ++vals_first;

        while (keys_first != keys_last) {
            if (!(*keys_first == prev_key)) {
                ++key_out;
                ++val_out;
                prev_key = *keys_first;
                *key_out = prev_key;
                *val_out = *vals_first;
            }
            ++keys_first;
            ++vals_first;
        }
        ++key_out;
        ++val_out;
        return { key_out, val_out };
    }
#ifdef TYVI_USE_HIP_BACKEND
    else {
        return thrust::unique_by_key(thrust::device,
            keys_first, keys_last, vals_first);
    }
#endif
}

} // namespace tyvi::algo
