#pragma once

#include <concepts>
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

} // namespace tyvi::sstd
