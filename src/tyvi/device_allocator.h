// Copyright 2026 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

#include "thrust/detail/raw_pointer_cast.h"
#include "thrust/device_allocator.h"
#include "thrust/device_ptr.h"

namespace tyvi {

/// Wraps thrust::device_allocator such that it strips thrust::device_{reference,ptr} and just uses plain pointers.
template<typename T>
struct [[nodiscard]] device_allocator {
  public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    constexpr device_allocator() = default;

    template<typename U>
    device_allocator(const device_allocator<U>&) {}

    pointer allocate(const size_type n) const {
        return thrust::raw_pointer_cast(thrust::device_allocator<T>{}.allocate(n));
    }

    void deallocate(const pointer p, const size_type n) const {
        thrust::device_allocator<T>{}.deallocate(thrust::device_pointer_cast(p), n);
    }

    template<typename U>
    struct rebind {
        using other = device_allocator<U>;
    };

    template<typename U, typename... Args>
    void construct(U* const p, Args&&... args) const {
        *thrust::device_pointer_cast(p) = U(std::forward<Args>(args)...);
    }
};

// Allocators are stateless, i.e. always equal.
template<typename T, typename U>
bool
operator==(const device_allocator<T>&, const device_allocator<U>&) {
    return true;
}

template<typename T, typename U>
bool
operator!=(const device_allocator<T>&, const device_allocator<U>&) {
    return false;
}

} // namespace tyvi
