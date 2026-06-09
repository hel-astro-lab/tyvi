// Copyright 2026 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <memory>
#include <print>
#include <vector>

#include "tyvi/mdsegments.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdsegments component views"> _ = [] {
    using T                 = int;
    static constexpr auto N = 4;
    using segments          = tyvi::
        mdsegments<T, N, tyvi::sstd::geometric_extents<2, 2>, std::layout_right, std::allocator<T>>;

    "component span is correct size for 2D elements"_test = [] {
        auto s = segments(5);

        const auto view00 = s.component_view<0, 0>();
        const auto view10 = s.component_cview<1, 0>();
        const auto view01 = s.component_cview<{ 1, 0 }>();
        const auto view11 = s.component_view<{ 1, 1 }>();
        expect(view00.size() == 5uz) << view00.size();
        expect(view10.size() == 5uz) << view10.size();
        expect(view01.size() == 5uz) << view01.size();
        expect(view11.size() == 5uz) << view11.size();
    };

    "modification through component span is correct"_test = [] {
        auto s = segments(5);

        const auto view00 = s.component_view<0, 0>();
        const auto view10 = s.component_cview<1, 0>();
        const auto view01 = s.component_cview<{ 0, 1 }>();
        const auto view11 = s.component_view<{ 1, 1 }>();

        std::ignore = std::ranges::fill(view00, 1);
        std::ignore = std::ranges::fill(view01, 2);
        std::ignore = std::ranges::fill(view10, 3);
        std::ignore = std::ranges::fill(view11, 4);

        const auto mds = s.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            expect(mds[idx][0, 0] == 1) << mds[idx][0, 0];
            expect(mds[idx][0, 1] == 2) << mds[idx][0, 1];
            expect(mds[idx][1, 0] == 3) << mds[idx][1, 0];
            expect(mds[idx][1, 1] == 4) << mds[idx][1, 1];
        }
    };

    "components are sortable"_test = [] {
        auto s = segments(5);

        {
            const auto v = s.raw_view();
            std::ranges::copy(std::views::iota(0, static_cast<int>(std::ranges::ssize(v)))
                                  | std::views::reverse,
                              v.begin());
        }

        auto ascending = [&] {
            const auto a = std::ranges::is_sorted(s.component_view<0, 0>());
            const auto b = std::ranges::is_sorted(s.component_cview<1, 0>());
            const auto c = std::ranges::is_sorted(s.component_cview<{ 0, 1 }>());
            const auto d = std::ranges::is_sorted(s.component_view<{ 1, 1 }>());
            return a and b and c and d;
        };

        expect(not ascending());

        std::ranges::sort(s.component_view<0, 0>());
        std::ranges::sort(s.component_cview<1, 0>());
        std::ranges::sort(s.component_cview<{ 0, 1 }>());
        std::ranges::sort(s.component_view<{ 1, 1 }>());

        expect(ascending());
    };

    "component_view is random access range"_test = [] {
        auto s = segments(24); // such that raw view is 24 elements.

        const auto v = s.component_view<1, 0>();
        expect(v.size() == 24);

        using iterator_type = std::ranges::iterator_t<decltype(v)>;
        using sentinel_type = std::ranges::sentinel_t<decltype(v)>;
        using range_type    = decltype(v);

        expect(std::random_access_iterator<iterator_type>);
        expect(std::sentinel_for<sentinel_type, iterator_type>);
        expect(std::ranges::random_access_range<range_type>);

        std::ranges::iota(v, 0);

        auto b = std::ranges::begin(v);
        auto e = std::ranges::end(v);

        auto test = [](const auto x, const auto e) {
            expect(x == e) << std::format("{} != {}", x, e);
        };

        test(*b++, 0);

        --b;

        test(*++b, 1);

        using difference_type = std::iter_difference_t<iterator_type>;

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

        expect(b + difference_type{ 17 } == e);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
