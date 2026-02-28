#pragma once

#include "tyvi/backend.h"

#ifdef TYVI_USE_CPU_BACKEND
#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>
#elif defined(TYVI_USE_HIP_BACKEND)
#include "thrust/iterator/counting_iterator.h"
#include "thrust/iterator/transform_iterator.h"
#include "thrust/iterator/transform_output_iterator.h"
#endif

namespace tyvi {

// =====================================================================
// counting_iterator
// =====================================================================

#ifdef TYVI_USE_CPU_BACKEND

template<typename T>
class counting_iterator {
    T value_{};

  public:
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    using reference         = T;
    using pointer           = const T*;

    constexpr counting_iterator() = default;
    constexpr explicit counting_iterator(T val) : value_(val) {}

    constexpr T operator*() const { return value_; }
    constexpr T operator[](difference_type n) const { return value_ + static_cast<T>(n); }

    constexpr counting_iterator& operator++() { ++value_; return *this; }
    constexpr counting_iterator  operator++(int) { auto t = *this; ++value_; return t; }
    constexpr counting_iterator& operator--() { --value_; return *this; }
    constexpr counting_iterator  operator--(int) { auto t = *this; --value_; return t; }

    constexpr counting_iterator& operator+=(difference_type n) {
        value_ += static_cast<T>(n);
        return *this;
    }
    constexpr counting_iterator& operator-=(difference_type n) {
        value_ -= static_cast<T>(n);
        return *this;
    }

    friend constexpr counting_iterator operator+(counting_iterator it, difference_type n) {
        return counting_iterator(it.value_ + static_cast<T>(n));
    }
    friend constexpr counting_iterator operator+(difference_type n, counting_iterator it) {
        return it + n;
    }
    friend constexpr counting_iterator operator-(counting_iterator it, difference_type n) {
        return counting_iterator(it.value_ - static_cast<T>(n));
    }
    friend constexpr difference_type operator-(counting_iterator a, counting_iterator b) {
        return static_cast<difference_type>(a.value_) - static_cast<difference_type>(b.value_);
    }

    constexpr bool operator==(const counting_iterator&) const  = default;
    constexpr auto operator<=>(const counting_iterator&) const = default;
};

#else // HIP backend

template<typename T>
using counting_iterator = thrust::counting_iterator<T>;

#endif

// =====================================================================
// transform_iterator (via make_transform_iterator)
// =====================================================================

#ifdef TYVI_USE_CPU_BACKEND

template<typename Iter, typename F>
class transform_iterator {
    Iter base_;
    F func_;

  public:
    using value_type        = std::invoke_result_t<F, std::iter_reference_t<Iter>>;
    using difference_type   = std::iter_difference_t<Iter>;
    using iterator_category = std::random_access_iterator_tag;
    using reference         = value_type;
    using pointer           = void;

    constexpr transform_iterator(Iter base, F func)
        : base_(base), func_(func) {}

    constexpr value_type operator*() const { return func_(*base_); }
    constexpr value_type operator[](difference_type n) const { return func_(base_[n]); }

    constexpr transform_iterator& operator++() { ++base_; return *this; }
    constexpr transform_iterator  operator++(int) { auto t = *this; ++base_; return t; }
    constexpr transform_iterator& operator--() { --base_; return *this; }
    constexpr transform_iterator  operator--(int) { auto t = *this; --base_; return t; }

    constexpr transform_iterator& operator+=(difference_type n) { base_ += n; return *this; }
    constexpr transform_iterator& operator-=(difference_type n) { base_ -= n; return *this; }

    friend constexpr transform_iterator operator+(transform_iterator it, difference_type n) {
        it.base_ += n;
        return it;
    }
    friend constexpr transform_iterator operator+(difference_type n, transform_iterator it) {
        return it + n;
    }
    friend constexpr transform_iterator operator-(transform_iterator it, difference_type n) {
        it.base_ -= n;
        return it;
    }
    friend constexpr difference_type operator-(transform_iterator a, transform_iterator b) {
        return a.base_ - b.base_;
    }

    constexpr bool operator==(const transform_iterator& o) const { return base_ == o.base_; }
    constexpr auto operator<=>(const transform_iterator& o) const { return base_ <=> o.base_; }
};

template<typename Iter, typename F>
constexpr auto
make_transform_iterator(Iter it, F f) {
    return transform_iterator<Iter, F>(it, f);
}

#else // HIP backend

template<typename Iter, typename F>
constexpr auto
make_transform_iterator(Iter it, F f) {
    return thrust::make_transform_iterator(it, f);
}

#endif

// =====================================================================
// transform_output_iterator (via make_transform_output_iterator)
// =====================================================================

#ifdef TYVI_USE_CPU_BACKEND

template<typename Iter, typename F>
class transform_output_iterator {
    Iter base_;
    F func_;

  public:
    using difference_type = std::ptrdiff_t;

    constexpr transform_output_iterator(Iter base, F func)
        : base_(base), func_(func) {}

    struct proxy {
        Iter pos_;
        F func_;

        template<typename U>
        constexpr proxy& operator=(U&& val) {
            *pos_ = func_(std::forward<U>(val));
            return *this;
        }
    };

    constexpr proxy operator*() { return { base_, func_ }; }

    constexpr transform_output_iterator& operator++() { ++base_; return *this; }
    constexpr transform_output_iterator  operator++(int) { auto t = *this; ++base_; return t; }

    friend constexpr difference_type operator-(
        transform_output_iterator a, transform_output_iterator b) {
        return a.base_ - b.base_;
    }
};

template<typename Iter, typename F>
constexpr auto
make_transform_output_iterator(Iter it, F f) {
    return transform_output_iterator<Iter, F>(it, f);
}

#else // HIP backend

template<typename Iter, typename F>
constexpr auto
make_transform_output_iterator(Iter it, F f) {
    return thrust::make_transform_output_iterator(it, f);
}

#endif

} // namespace tyvi
