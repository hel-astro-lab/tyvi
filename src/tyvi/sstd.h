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
    for (auto _ : std::views::iota(T{ 0 }, exponent)) { result *= base; }
    return result;
}

} // namespace tyvi::sstd
