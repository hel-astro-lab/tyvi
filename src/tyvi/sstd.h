// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <stdexcept>

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

/// CRTP type for implementing iterators based on offsets.
///
/// T is the type that derives offset_iterator.
/// T has to only define default ctor, T::offset_dereference and comparsion operator.
/// T::offset_dereference  has to be invocable with Distance
/// and its return value has to convertible to Reference.
template<typename T,
         typename ValueType,
         typename Distance  = std::ptrdiff_t,
         typename Reference = ValueType&>
class offset_iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = ValueType;
    using difference_type   = Distance;
    using pointer           = void;
    using reference         = Reference;

  private:
    constexpr offset_iterator() = default;
    explicit constexpr offset_iterator(const difference_type n) : offset_{ n } {}
    friend T;

    static constexpr T& lower(offset_iterator& p) { return static_cast<T&>(p); }
    static constexpr const T& lower(const offset_iterator& p) { return static_cast<const T&>(p); }

  public:
    /// prefix increment
    constexpr T& operator++() {
        ++offset_;
        return *static_cast<T*>(this);
    }

    /// postfix increment
    [[nodiscard]]
    constexpr T operator++(int) {
        T old = *static_cast<T*>(this);
        ++(*static_cast<T*>(this));
        return old;
    }

    /// prefix decrement
    constexpr T& operator--() {
        --offset_;
        return *static_cast<T*>(this);
    }

    /// postfix decrement
    [[nodiscard]]
    constexpr T operator--(int) {
        T old = *static_cast<T*>(this);
        --(*static_cast<T*>(this));
        return old;
    }

    [[nodiscard]]
    constexpr reference operator*() const {
        return lower(*this).offset_dereference(this->offset_);
    }

    [[nodiscard]]
    constexpr auto operator<=>(const offset_iterator& rhs) const = default;

    [[nodiscard]]
    friend constexpr difference_type operator-(const offset_iterator& lhs,
                                               const offset_iterator& rhs) {
        if (lhs.offset_ >= rhs.offset_) {
            return static_cast<difference_type>(lhs.offset_ - rhs.offset_);
        }
        return -static_cast<difference_type>(rhs.offset_ - lhs.offset_);
    }

    [[nodiscard]]
    friend constexpr T operator+(const T& lhs, const difference_type rhs) {
        auto result = lhs;
        static_cast<offset_iterator&>(result).offset_ =
            static_cast<const offset_iterator&>(lhs).offset_ + rhs;
        return result;
    }

    [[nodiscard]]
    friend constexpr T operator+(const difference_type lhs, const T& rhs) {
        return rhs + lhs;
    }

    [[nodiscard]]
    friend constexpr T operator-(const T& lhs, const difference_type rhs) {
        return lhs + (-rhs);
    }

    constexpr T& operator+=(const difference_type rhs) {
        return *static_cast<T*>(this) = *static_cast<T*>(this) + rhs;
    }

    constexpr T& operator-=(const difference_type rhs) {
        return *static_cast<T*>(this) = *static_cast<T*>(this) - rhs;
    }

    [[nodiscard]]
    constexpr reference operator[](const difference_type rhs) const {
        return *(*static_cast<T const*>(this) + rhs);
    }

  private:
    difference_type offset_{ 0 };
};

} // namespace tyvi::sstd
