// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <cstddef>
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

    "raw_view is random access range"_test = [] {
        auto s       = segments(6); // such that raw view is 24 elements.
        const auto v = s.raw_view();
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

        const auto& cs  = static_cast<const segments&>(s);
        const auto cmds = cs.mds();

        expect(std::is_const_v<
               std::remove_reference_t<typename decltype(cmds)::element_type::reference>>);

        for (const auto idx : tyvi::sstd::index_space(cmds)) {
            for (const auto tidx : tyvi::sstd::index_space(cmds[idx])) {
                expect(cmds[idx][tidx] == bad_hash(idx[0], tidx[0], tidx[1]));
            }
        }
    };

    "host to device copy and device to host copy"_test = [] {
        auto host   = segments(3);
        auto device = device_segments(0);

        const auto hmds = host.mds();

        for (const auto idx : tyvi::sstd::index_space(hmds)) {
            for (const auto tidx : tyvi::sstd::index_space(hmds[idx])) { hmds[idx][tidx] = 21; }
        }

        tyvi::h2d_copy(host, device);

        const auto dmds = device.mds();
        tyvi::mdgrid_work()
            .for_each_index(
                dmds,
                [=](const auto idx, const auto tidx) { dmds[idx][tidx] = 2 * dmds[idx][tidx]; })
            .wait();

        tyvi::d2h_copy(device, host);

        for (const auto idx : tyvi::sstd::index_space(hmds)) {
            for (const auto tidx : tyvi::sstd::index_space(hmds[idx])) {
                expect(hmds[idx][tidx] == 42) << hmds[idx][tidx];
            }
        }
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
