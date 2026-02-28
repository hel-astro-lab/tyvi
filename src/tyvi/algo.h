#pragma once

#include <algorithm>

#include "tyvi/backend.h"

#ifdef TYVI_USE_HIP_BACKEND
#include "thrust/copy.h"
#include "thrust/execution_policy.h"
#include "thrust/for_each.h"
#include "thrust/sort.h"
#endif

namespace tyvi::algo {

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

} // namespace tyvi::algo
