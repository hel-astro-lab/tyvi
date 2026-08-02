// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include "constant_testing.h"

namespace {
constexpr auto
sum(auto... values) {
    return (values + ...);
}

auto
call_expect_with(const bool x) {
    boost::ut::expect(x);
}
} // namespace

int
main(int argc, const char** argv) {
    using namespace boost::ut;
    [[maybe_unused]]
    const suite<"unit testing"> _ = [] {
        "sum"_test = [] {
            expect(sum(0) == 0_i);
            expect(sum(1, 2) == 3_i);
            expect(sum(1, 2) > 0_i and 4_i == sum(2, 2));
        };

        "subfunctions"_test = [] { call_expect_with(true); };

        "constant testing"_test = []() {
            tyvi::constant_testing([](auto& tester) static consteval {
                const auto foo = 42;
                tester.expect(foo == 42_i);
            });
        };
    };

    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
