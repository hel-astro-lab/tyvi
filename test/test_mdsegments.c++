// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <print>
#include <ranges>
#include <type_traits>
#include <vector>

#include "tyvi/device_allocator.h"
#include "tyvi/mdgrid.h"
#include "tyvi/mdsegments.h"
#include "tyvi/mdspan.h"
#include "tyvi/sstd.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdsegments"> _ = [] {
    using T                 = int;
    static constexpr auto N = 3;
    using E                 = tyvi::sstd::geometric_extents<2, 2>;
    using LP                = std::layout_right;
    using allocator         = std::allocator<T>;
    using segments          = tyvi::mdsegments<T, N, E, LP, allocator>;
    using device_segments   = tyvi::mdsegments<T, N, E, LP, tyvi::device_allocator<T>>;

    "default constructed is empty"_test = [] {
        auto s = segments();
        expect(s.empty());
    };

    "size can be changed"_test = [] {
        auto s = segments(10);
        expect(not s.empty());
        expect(s.size() == 10uz);
        s.resize(17);
        expect(s.size() == 17uz);
    };

    "raw_view"_test = [] {
        auto s = segments(17);

        expect(s.raw_view().size() == 18 * LP::mapping<E>{}.required_span_size());

        for (auto& x : s.raw_view()) { x = 42; }
        for (const auto& x : static_cast<const segments&>(s).raw_view()) { expect(x == 42); }

        expect(std::ranges::view<decltype(s.raw_view())>);
    };

    "resize does not invalidate references"_test = [] {
        auto s                 = segments(10);
        const auto to_pointers = std::views::transform([](const auto& x) { return &x; });

        const auto a = s.raw_view() | to_pointers | std::ranges::to<std::vector>();
        s.resize(10000);
        const auto b = s.raw_view() | to_pointers | std::views::take(std::ranges::size(a))
                       | std::ranges::to<std::vector>();
        expect(a == b);
    };

    "writeable through mdspan"_test = [] {
        auto s         = segments(15);
        const auto mds = s.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 42; }
        }

        expect(std::ranges::all_of(s.raw_view(), [](const auto x) { return x == 42; }));
    };

    "const correct viewable through mdspan"_test = [] {
        auto s         = segments(7);
        const auto mds = s.mds();

        expect(not std::is_const_v<
               std::remove_reference_t<typename decltype(mds)::element_type::reference>>);

        auto bad_hash = [](const auto n, const auto i, const auto j) {
            return static_cast<int>(4 * n + 2 * i + j);
        };

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) {
                mds[idx][tidx] = bad_hash(idx[0], tidx[0], tidx[1]);
            }
        }

        const auto cmds = s.cmds();

        expect(std::is_const_v<
               std::remove_reference_t<typename decltype(cmds)::element_type::reference>>);

        for (const auto idx : tyvi::sstd::index_space(cmds)) {
            for (const auto tidx : tyvi::sstd::index_space(cmds[idx])) {
                expect(cmds[idx][tidx] == bad_hash(idx[0], tidx[0], tidx[1]));
            }
        }
    };

    "copyinig"_test = [] {
        auto host1   = segments(3);
        auto host2   = segments(3);
        auto device1 = device_segments(0);
        auto device2 = device_segments(0);

        const auto hmds = host1.mds();

        for (const auto idx : tyvi::sstd::index_space(hmds)) {
            for (const auto tidx : tyvi::sstd::index_space(hmds[idx])) { hmds[idx][tidx] = 21; }
        }

        tyvi::copy_h2h(host1, host2);
        tyvi::copy_h2d(host2, device1);

        const auto dmds = device1.mds();
        tyvi::mdgrid_work()
            .for_each_index(
                dmds,
                [=](const auto idx, const auto tidx) { dmds[idx][tidx] = 2 * dmds[idx][tidx]; })
            .wait();

        tyvi::copy_d2d(device1, device2);
        tyvi::copy_d2h(device2, host1);

        for (const auto idx : tyvi::sstd::index_space(hmds)) {
            for (const auto tidx : tyvi::sstd::index_space(hmds[idx])) {
                expect(hmds[idx][tidx] == 42) << hmds[idx][tidx];
            }
        }
    };

    "submdspans"_test = [] {
        auto s   = segments(10);
        auto mds = s.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 21; }
        }

        const auto submds =
            std::submdspan(mds, std::strided_slice{ .offset = 2uz, .extent = 6uz, .stride = 3uz });

        for (const auto idx : tyvi::sstd::index_space(submds)) {
            const auto innersubmds =
                std::submdspan(submds[idx], std::tuple{ 1uz, 2uz }, std::tuple{ 1uz, 2uz });
            for (const auto tidx : tyvi::sstd::index_space(innersubmds)) { innersubmds[tidx] = 42; }
        }

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) {
                const auto n = idx[0];
                const auto i = tidx[0];
                const auto j = tidx[1];

                const auto idx_ok  = n == 2uz or n == 5uz;
                const auto tidx_ok = i == 1uz and j == 1uz;

                const auto debug_str = std::format("[{}][{}, {}] = {}", n, i, j, mds[idx][tidx]);

                if (idx_ok and tidx_ok) {
                    expect(mds[idx][tidx] == 42) << debug_str;
                } else {
                    expect(mds[idx][tidx] == 21) << debug_str;
                }
            }
        }
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
