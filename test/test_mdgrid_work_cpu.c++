#include <boost/ut.hpp> // import boost.ut;

#include <utility>

#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_work_cpu"> _ = [] {
    "mdgrid_work is constructible and waitable"_test = [] {
        const auto w = tyvi::mdgrid_work{};
        w.wait();
        expect(true);
    };

    "work can split to one"_test = [] {
        expect(nothrow([] {
            const auto w1   = tyvi::mdgrid_work{};
            const auto [w2] = w1.split<1>();

            tyvi::when_all(w1, w2);
            w1.wait();
        }));
    };

    "work can split to many"_test = [] {
        expect(nothrow([] {
            const auto w1           = tyvi::mdgrid_work{};
            const auto [w2, w3, w4] = w1.split<3>();

            tyvi::when_all(w1, w2, w3, w4);
            w1.wait();
        }));
    };

    "work can split multiple times"_test = [] {
        expect(nothrow([] {
            const auto w1         = tyvi::mdgrid_work{};
            const auto [w2a, w2b] = w1.split<2>();
            const auto [w3a, w3b] = w2a.split<2>();
            const auto [w3c, w3d] = w2b.split<2>();

            tyvi::when_all(w1, w2a, w2b, w3a, w3b, w3c, w3d);
            w1.wait();
        }));
    };

    "work can be moved out of scope"_test = [] {
        expect(nothrow([] {
            auto w1 = tyvi::mdgrid_work{};
            {
                auto w2 = tyvi::mdgrid_work{};
                w1      = std::move(w2);
            }
            w1.wait();
        }));
    };

    "work can be moved constructed"_test = [] {
        expect(nothrow([] {
            auto w1 = tyvi::mdgrid_work{};
            const auto w2{ std::move(w1) };
            w2.wait();
        }));
    };

    "for_each modifies grid"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(3, 4, 5);

        auto smds = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(smds)) {
            for (const auto jdx : tyvi::sstd::index_space(smds[idx])) { smds[idx][jdx] = 10; }
        }

        const auto w = tyvi::mdgrid_work{};
        w.for_each(grid, [](const auto& M) {
            for (const auto jdx : tyvi::sstd::index_space(M)) { M[jdx] = M[jdx] * 2; }
        });
        w.wait();

        for (const auto idx : tyvi::sstd::index_space(smds)) {
            for (const auto jdx : tyvi::sstd::index_space(smds[idx])) {
                expect(smds[idx][jdx] == 20);
            }
        }
    };

    "for_each_index grid-only"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(4, 4, 4);

        const auto w = tyvi::mdgrid_work{};
        w.for_each_index(grid, [gmds = grid.mds()](const auto& idx) {
            gmds[idx][0] = static_cast<int>(idx[0]);
            gmds[idx][1] = static_cast<int>(idx[1]);
            gmds[idx][2] = static_cast<int>(idx[2]);
        });
        w.wait();

        const auto mds = grid.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            expect(mds[idx][0] == static_cast<int>(idx[0]));
            expect(mds[idx][1] == static_cast<int>(idx[1]));
            expect(mds[idx][2] == static_cast<int>(idx[2]));
        }
    };

    "for_each_index grid+element"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(3, 4, 5);

        const auto w = tyvi::mdgrid_work{};
        w.for_each_index(grid, [gmds = grid.mds()](const auto& idx, const auto& tidx) {
            gmds[idx][tidx] = static_cast<int>(tidx[0] * 10 + tidx[1]);
        });
        w.wait();

        const auto mds = grid.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) {
                expect(mds[idx][tidx] == static_cast<int>(tidx[0] * 10 + tidx[1]));
            }
        }
    };

    "sync_to/from_staging are no-ops"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(2, 2, 2);

        auto mds = grid.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto jdx : tyvi::sstd::index_space(mds[idx])) { mds[idx][jdx] = 99; }
        }

        const auto w = tyvi::mdgrid_work{};
        // These should be no-ops
        w.sync_to_staging(grid).sync_from_staging(grid).wait();

        // Data should be unchanged
        auto smds = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(smds)) {
            for (const auto jdx : tyvi::sstd::index_space(smds[idx])) {
                expect(smds[idx][jdx] == 99);
            }
        }
    };

    "TYVI_CMDS macro works"_test = [] {
        constexpr auto scalar_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 0, .dim = 3 };
        constexpr auto vec_desc    = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };

        using scalar_mdg = tyvi::mdgrid<scalar_desc, std::dextents<std::size_t, 3>>;
        using vec_mdg    = tyvi::mdgrid<vec_desc, std::dextents<std::size_t, 3>>;

        auto scalar_grid = scalar_mdg(2, 2, 2);
        auto vec_grid    = vec_mdg(2, 2, 2);

        // Initialize scalar grid
        auto smds = scalar_grid.mds();
        for (const auto idx : tyvi::sstd::index_space(smds)) { smds[idx][] = 5; }

        const auto w = tyvi::mdgrid_work{};
        w.for_each_index(vec_grid, [TYVI_CMDS(vec_grid, scalar_grid)](const auto& idx) {
            for (const auto jdx : tyvi::sstd::index_space(vec_grid_mds[idx])) {
                vec_grid_mds[idx][jdx] = scalar_grid_mds[idx][];
            }
        });
        w.wait();

        auto vmds = vec_grid.mds();
        for (const auto idx : tyvi::sstd::index_space(vmds)) {
            for (const auto jdx : tyvi::sstd::index_space(vmds[idx])) {
                expect(vmds[idx][jdx] == 5);
            }
        }
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
