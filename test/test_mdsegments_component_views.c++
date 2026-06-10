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
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
