#pragma once

#include "tyvi/backend.h"

#ifdef TYVI_USE_CPU_BACKEND
#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#elif defined(TYVI_USE_HIP_BACKEND)
#include "thrust/iterator/counting_iterator.h"
#include "thrust/iterator/transform_iterator.h"
#include "thrust/iterator/transform_output_iterator.h"
#include "thrust/iterator/zip_iterator.h"
#include "thrust/tuple.h"
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

    constexpr transform_iterator() = default;

    constexpr transform_iterator(Iter base, F func)
        : base_(base), func_(func) {}

    constexpr transform_iterator(const transform_iterator&) = default;
    constexpr transform_iterator(transform_iterator&&) noexcept = default;

    // Lambdas are not assignable, so we use destroy+construct to enable assignment.
    constexpr transform_iterator& operator=(const transform_iterator& other) {
        if (this != &other) {
            std::destroy_at(this);
            std::construct_at(this, other);
        }
        return *this;
    }
    constexpr transform_iterator& operator=(transform_iterator&& other) noexcept {
        if (this != &other) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }
        return *this;
    }

    constexpr ~transform_iterator() = default;

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

// =====================================================================
// zip_iterator (via make_zip_iterator)
// =====================================================================

#ifdef TYVI_USE_CPU_BACKEND

template<typename Iter1, typename Iter2>
class zip_iterator {
    Iter1 it1_;
    Iter2 it2_;

  public:
    using value_type        = std::tuple<std::iter_value_t<Iter1>, std::iter_value_t<Iter2>>;
    using reference         = std::tuple<std::iter_reference_t<Iter1>, std::iter_reference_t<Iter2>>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    using pointer           = void;

    constexpr zip_iterator(Iter1 it1, Iter2 it2) : it1_(it1), it2_(it2) {}

    constexpr reference operator*() const { return { *it1_, *it2_ }; }
    constexpr reference operator[](difference_type n) const { return { it1_[n], it2_[n] }; }

    constexpr zip_iterator& operator++() { ++it1_; ++it2_; return *this; }
    constexpr zip_iterator  operator++(int) { auto t = *this; ++it1_; ++it2_; return t; }
    constexpr zip_iterator& operator--() { --it1_; --it2_; return *this; }
    constexpr zip_iterator  operator--(int) { auto t = *this; --it1_; --it2_; return t; }

    constexpr zip_iterator& operator+=(difference_type n) { it1_ += n; it2_ += n; return *this; }
    constexpr zip_iterator& operator-=(difference_type n) { it1_ -= n; it2_ -= n; return *this; }

    friend constexpr zip_iterator operator+(zip_iterator it, difference_type n) {
        it.it1_ += n; it.it2_ += n;
        return it;
    }
    friend constexpr zip_iterator operator+(difference_type n, zip_iterator it) {
        return it + n;
    }
    friend constexpr zip_iterator operator-(zip_iterator it, difference_type n) {
        it.it1_ -= n; it.it2_ -= n;
        return it;
    }
    friend constexpr difference_type operator-(zip_iterator a, zip_iterator b) {
        return a.it1_ - b.it1_;
    }

    constexpr bool operator==(const zip_iterator& o) const { return it1_ == o.it1_; }
    constexpr auto operator<=>(const zip_iterator& o) const { return it1_ <=> o.it1_; }
};

template<typename Iter1, typename Iter2>
constexpr auto
make_zip_iterator(Iter1 it1, Iter2 it2) {
    return zip_iterator<Iter1, Iter2>(it1, it2);
}

#else // HIP backend

template<typename Iter1, typename Iter2>
constexpr auto
make_zip_iterator(Iter1 it1, Iter2 it2) {
    return thrust::make_zip_iterator(thrust::make_tuple(it1, it2));
}

#endif

} // namespace tyvi
