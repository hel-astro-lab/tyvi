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
    static constexpr auto N = 37;
    using segments          = tyvi::
        mdsegments<T, N, tyvi::sstd::geometric_extents<2, 2>, std::layout_right, std::allocator<T>>;

    "component span is correct size for 2D elements"_test = [] {
        auto s = segments(87);

        const auto view00 = s.component_view<0, 0>();
        const auto view10 = s.component_cview<1, 0>();
        const auto view01 = s.component_cview<{ 1, 0 }>();
        const auto view11 = s.component_view<{ 1, 1 }>();
        expect(view00.size() == 87uz) << view00.size();
        expect(view10.size() == 87uz) << view10.size();
        expect(view01.size() == 87uz) << view01.size();
        expect(view11.size() == 87uz) << view11.size();
    };

    "modification through component span is correct"_test = [] {
        auto s = segments(98uz);

        const auto view00 = s.component_view<0, 0>();
        const auto view10 = s.component_cview<1, 0>();
        const auto view01 = s.component_cview<{ 0, 1 }>();
        const auto view11 = s.component_view<{ 1, 1 }>();

        for (auto i = 0; i < 98; ++i) {
            view00[i] = i;
            view01[i] = i * 100;
            view10[i] = i * 10000;
            view11[i] = i * 1000000;
        }

        const auto mds = s.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            expect(std::cmp_equal(mds[idx][0, 0], idx[0])) << mds[idx][0, 0];
            expect(std::cmp_equal(mds[idx][0, 1], idx[0] * 100uz)) << mds[idx][0, 1];
            expect(std::cmp_equal(mds[idx][1, 0], idx[0] * 10000uz)) << mds[idx][1, 0];
            expect(std::cmp_equal(mds[idx][1, 1], idx[0] * 1000000uz)) << mds[idx][1, 1];
        }
    };

    "components are sortable"_test = [] {
        auto s = segments(1002);

        {
            const auto v = s.raw_view();
            std::ranges::copy(std::views::iota(0, static_cast<int>(std::ranges::ssize(v)))
                                  | std::views::reverse,
                              v.begin());
        }

        auto ascending = [&] {
            const auto mds = s.mds();

            for (const auto i : std::views::iota(1, static_cast<int>(s.size()))) {
                if (mds[i][0, 0] <= mds[i - 1][0, 0]) { return false; }
                if (mds[i][1, 0] <= mds[i - 1][1, 0]) { return false; }
                if (mds[i][0, 1] <= mds[i - 1][0, 1]) { return false; }
                if (mds[i][1, 1] <= mds[i - 1][1, 1]) { return false; }
            }

            return true;
        };

        expect(not ascending());
        std::ranges::sort(s.component_view<0, 0>());
        expect(not ascending());
        std::ranges::sort(s.component_cview<1, 0>());
        expect(not ascending());
        std::ranges::sort(s.component_cview<{ 0, 1 }>());
        expect(not ascending());
        std::ranges::sort(s.component_view<{ 1, 1 }>());
        expect(ascending());
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
