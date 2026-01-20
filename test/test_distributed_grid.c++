
#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <set>

#include "tyvi/distributed_grid.h"
#include "tyvi/execution.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"distributed_grid"> _ = [] {
    "rank 3 distributed_grid can be constructed"_test = [] {
        using E = std::dextents<std::size_t, 3>;
        expect(nothrow([] {
            [[maybe_unused]]
            const auto _ = tyvi::distributed_grid<std::size_t, E>(3, 4, 5);
        }));
    };

    "extents of distributed_grid is quariable"_test = [] {
        using E = std::dextents<std::size_t, 3>;
        expect(nothrow([] {
            const auto grid = tyvi::distributed_grid<std::size_t, E>(3, 4, 5);

            expect(grid.extents() == E{ 3, 4, 5 });
        }));
    };

    "all tiles are local tiles with one rank"_test = [] {
        using E         = std::dextents<std::size_t, 3>;
        using arr       = std::array<std::size_t, 3>;
        const auto grid = tyvi::distributed_grid<std::size_t, E>(3, 4, 5);

        auto indices = std::set<arr>{};

        for (const auto& idx : grid.local_indices()) {
            const auto idx_arr = idx;

            expect(not std::ranges::contains(indices, idx_arr));
            indices.insert(idx_arr);
        }

        const auto m = std::layout_left::mapping<E>{ E{ 3, 4, 5 } };

        for (const auto idx : tyvi::sstd::index_space(m)) {
            expect(std::ranges::contains(indices, arr{ idx }));
        }

        expect(std::ranges::size(indices) == 3uz * 4uz * 5uz);
    };

    "for_each can read and write"_test = [] {
        using E   = std::dextents<std::size_t, 3>;
        auto grid = tyvi::distributed_grid<std::size_t, E>(3, 4, 5);

        grid.for_each([](auto h) { h.value = idx })

            auto write =
            grid.pairwise<std::size_t, { -1 }>([]() { t = idx[0] + 10 * idx[1] + 100 * idx[2]; });

        auto read = grid.for_each<std::size_t>([](const auto idx, std::size_t& t) {
            expect(t == idx[0] + 10 * idx[1] + 100 * idx[2]);
        });

        tyvi::this_thread::sync_wait();

        const auto& cgrid = grid;
        for (const auto& idx : grid.local_indices()) {
            tyvi::this_thread::sync_wait(cgrid.get<std::size_t>(idx)
                                         | tyvi::exec::then([=](const std::size_t& t) { ; }));
        }
    };

    "local tiles can be assinged and read with explicit indices"_test = [] {
        using E   = std::extents<std::size_t, 2uz, 2uz>;
        auto grid = tyvi::distributed_grid<std::size_t, E>();

        tyvi::this_thread::sync_wait(grid.get<std::size_t>(0, 0)
                                     | tyvi::exec::then([](std::size_t& t) { t = 42; }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(1, 0)
                                     | tyvi::exec::then([](std::size_t& t) { t = 43; }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(0, 1)
                                     | tyvi::exec::then([](std::size_t& t) { t = 44; }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(1, 1)
                                     | tyvi::exec::then([](std::size_t& t) { t = 45; }));

        tyvi::this_thread::sync_wait(grid.get<std::size_t>(0, 0)
                                     | tyvi::exec::then([](std::size_t& t) { expect(t == 42); }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(1, 0)
                                     | tyvi::exec::then([](std::size_t& t) { expect(t == 43); }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(0, 1)
                                     | tyvi::exec::then([](std::size_t& t) { expect(t == 44); }));
        tyvi::this_thread::sync_wait(grid.get<std::size_t>(1, 1)
                                     | tyvi::exec::then([](std::size_t& t) { expect(t == 45); }));
    };

    "distributed_grid sender ctor"_test = [] {
        using E       = std::dextents<std::size_t, 3>;
        auto grid_snd = tyvi::make_distributed_grid<std::size_t, E>(3, 4, 5);

        const auto grid = tyvi::this_thread::sync_wait(std::move(grid_snd));
        expect(grid.extents() == E{ 3, 4, 5 });
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
