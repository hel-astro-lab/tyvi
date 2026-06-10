// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include <concepts>
#include <iterator>
#include <ranges>
#include <vector>

#include "tyvi/sstd.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"sstd::offset_iterator<T>"> _ = [] {
    "random access semantics"_test = [] {
        struct test_iterator : tyvi::sstd::offset_iterator<test_iterator, int> {
            int* ptr{ nullptr }; // NOLINT

            constexpr test_iterator() = default;
            constexpr test_iterator(int* const p, const difference_type n)
                : offset_iterator(n),
                  ptr(p) {}

            [[nodiscard]]
            constexpr reference offset_dereference(const difference_type offset) const {
                return *std::ranges::next(ptr, offset);
            }

            [[nodiscard]]
            constexpr bool operator==(const test_iterator&) const = default;
        };

        static_assert(std::random_access_iterator<test_iterator>);
        static_assert(std::same_as<int&, std::iter_reference_t<test_iterator>>);
        using difference_type = std::iter_difference_t<test_iterator>;
        static_assert(std::same_as<std::ptrdiff_t, difference_type>);
        static_assert(std::same_as<int, std::iter_value_t<test_iterator>>);

        auto vec = std::vector<int>(24);
        std::ranges::iota(vec, 0);

        auto b = test_iterator(vec.data(), 0);
        auto e = test_iterator(vec.data(), 24);

        auto test = [](const auto x, const auto y) {
            expect(x == y) << std::format("{} != {}", x, y);
        };

        test(*b++, 0);

        --b;

        test(*++b, 1);

        const auto n10 = difference_type{ 10 };
        const auto n4  = difference_type{ 4 };

        b = b + n10;

        test(*b, 11);

        b += n4;

        test(*b, 15);

        b = n4 + b;

        test(*b, 19);

        const auto m = e - b;

        test(m, difference_type{ 5 });

        b -= m;

        test(*b--, 14);
        test(*--b, 12);

        b = b - m;

        test(*b, 7);

        test(b[n10], 17);

        expect(b < e);
        expect(b <= e);
        expect(not(b > e));
        expect(not(b >= e));
        expect(b == b); // NOLINT
        expect(e == e); // NOLINT
        expect(b != e);

        expect(b + test_iterator::difference_type{ 17 } == e);
    };

    "random access semantics of functional iterator"_test = [] {
        struct test_iterator : tyvi::sstd::offset_iterator<test_iterator, int, int, int> {
            constexpr test_iterator() = default;
            explicit constexpr test_iterator(const difference_type n) : offset_iterator(n) {}

            [[nodiscard]]
            static constexpr reference offset_dereference(const difference_type offset) {
                return offset;
            }

            [[nodiscard]]
            constexpr bool operator==(const test_iterator&) const = default;
        };

        static_assert(std::random_access_iterator<test_iterator>);
        static_assert(std::same_as<int, std::iter_reference_t<test_iterator>>);
        using difference_type = std::iter_difference_t<test_iterator>;
        static_assert(std::same_as<int, difference_type>);
        static_assert(std::same_as<int, std::iter_value_t<test_iterator>>);

        auto b = test_iterator{};
        auto e = test_iterator{ 24 };

        auto test = [](const auto x, const auto y) {
            expect(x == y) << std::format("{} != {}", x, y);
        };

        test(*b++, 0);

        --b;

        test(*++b, 1);

        const auto n10 = difference_type{ 10 };
        const auto n4  = difference_type{ 4 };

        b = b + n10;

        test(*b, 11);

        b += n4;

        test(*b, 15);

        b = n4 + b;

        test(*b, 19);

        const auto m = e - b;

        test(m, difference_type{ 5 });

        b -= m;

        test(*b--, 14);
        test(*--b, 12);

        b = b - m;

        test(*b, 7);

        test(b[n10], 17);

        expect(b < e);
        expect(b <= e);
        expect(not(b > e));
        expect(not(b >= e));
        expect(b == b); // NOLINT
        expect(e == e); // NOLINT
        expect(b != e);

        expect(b + test_iterator::difference_type{ 17 } == e);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
