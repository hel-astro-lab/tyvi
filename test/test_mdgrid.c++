#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>

#include <experimental/mdspan>

#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid"> _ = [] {
    "3D mdgrid is constructible"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<float>{ .rank = 2, .dim = 2 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        [[maybe_unused]]
        auto _ = mdg(3, 4, 5);
        expect(true);
    };

    "mdgrid can init work"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 2 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(3, 4, 5);

        auto w1 = grid.init_work();
        w1.wait();

        expect(true);
    };

    "mdgrid staging buffer round trip"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(3, 4, 5);

        auto staging_mds_A = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_A)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_A[idx])) {
                staging_mds_A[idx][Midx] = 7;
            }
        }

        auto w1 = grid.init_work();
        auto w2 = w1.sync_from_staging(grid);
        auto w3 = w2.for_each(grid, [](const auto& M) {
            for (const auto Midx : tyvi::sstd::index_space(M)) { M[Midx] = M[Midx] * 3 * 2; }
        });
        auto w4 = w3.sync_to_staging(grid);

        w4.wait();

        auto staging_mds_B = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_B)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_B[idx])) {
                expect(staging_mds_B[idx][Midx] == 42);
            }
        }
    };

    "mdgrid is constructible from other mdgrid's extents"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };
        using mdg                = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto gridA = mdg(3, 4, 5);

        const auto EA = gridA.extents();

        auto gridB = mdg(EA);
        auto gridC = mdg(gridB.extents());

        const auto EB = gridB.extents();
        const auto EC = gridC.extents();

        expect(EA == decltype(EA){ 3, 4, 5 });
        expect(EA == EB);
        expect(EB == EC);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
