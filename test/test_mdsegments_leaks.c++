// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include <bit>
#include <cstddef>
#include <format>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <utility>

#include "tyvi/mdsegments.h"
#include "tyvi/mdspan.h"
#include "tyvi/sstd.h"

namespace {
using namespace boost::ut;

// NOLINTBEGIN{cppcoreguidelines-avoid-non-const-global-variables}
std::unordered_map<std::size_t, std::size_t> track_data;
// NOLINTEND{cppcoreguidelines-avoid-non-const-global-variables}

void
check() {
    for (const auto& [p, n] : track_data) {
        expect(n == 0) << std::format("{} unallocated bytes at {}", n, p);
    }
    track_data = std::unordered_map<std::size_t, std::size_t>{};
}

template<typename T>
struct [[nodiscard]] tracked_allocator {
    using value_type = T;

    tracked_allocator() = default;

    template<typename U>
    explicit tracked_allocator(const tracked_allocator<U>&) {}

    T* allocate(const std::size_t n) {
        auto* const p = std::allocator<T>{}.allocate(n);
        expect(track_data[std::bit_cast<std::size_t>(p)] == 0uz);
        track_data[std::bit_cast<std::size_t>(p)] = n * sizeof(T);
        return p;
    }

    void deallocate(T* const p, const std::size_t n) {
        expect(p != nullptr);
        expect(track_data[std::bit_cast<std::size_t>(p)] == sizeof(T) * n);
        track_data[std::bit_cast<std::size_t>(p)] -= sizeof(T) * n;
        std::allocator<T>{}.deallocate(p, n);
    }
};

template<typename T, typename U>
bool
operator==(const tracked_allocator<T>&, const tracked_allocator<U>&) {
    return true;
}

template<typename T, typename U>
bool
operator!=(const tracked_allocator<T>&, const tracked_allocator<U>&) {
    return false;
}

[[maybe_unused]]
const suite<"mdsegments memory leaks"> _ = [] {
    using T                 = int;
    static constexpr auto N = 63;
    using segments          = tyvi::mdsegments<T,
                                               N,
                                               tyvi::sstd::geometric_extents<2, 2>,
                                               std::layout_right,
                                               tracked_allocator<T>>;

    "empty ctor and dtor"_test = [] {
        { auto s = segments(); }
        check();
    };

    "explicitly empty ctor and dtor"_test = [] {
        { auto s = segments(0); }
        check();
    };

    "ctor and dtor"_test = [] {
        { auto s = segments(1000); }
        check();
    };

    "move assignment"_test = [] {
        {
            auto s1 = segments(1000);
            [[maybe_unused]]
            auto _ = std::move(s1);
        }
        check();
    };

    "move ctor"_test = [] {
        {
            auto s1 = segments(1000);
            [[maybe_unused]]
            const segments _{ std::move(s1) };
        }
        check();
    };

    "2x move"_test = [] {
        {
            auto s3 = segments();
            {
                auto s2 = segments(10);
                {
                    auto s1 = segments(1000);
                    s2      = std::move(s1);
                }
                s3 = std::move(s2);
            }
        }
        check();
    };

    "resize"_test = [] {
        {
            auto s1 = segments();
            auto s2 = segments();
            for (const auto n : std::views::iota(0uz, 10uz)) {
                s1.resize(n * 100uz);
                s2 = std::move(s1);
                s1 = std::move(s2);
            }

            for (const auto n : std::views::iota(0uz, 10uz) | std::views::reverse) {
                s1.resize(n * 100uz);
                s2 = std::move(s1);
                s1 = std::move(s2);
            }
        }
        check();
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
