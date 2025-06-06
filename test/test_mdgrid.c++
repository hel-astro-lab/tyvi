#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <utility>

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

    "mdgrid_work is constructible and waitable"_test = [] {
        auto w1 = tyvi::mdgrid_work{};
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

        auto w1 = tyvi::mdgrid_work{};
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

    "multiple mdgrids in the same kernel with *::for_each_index"_test = [] {
        constexpr auto scalar_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 0, .dim = 3 };
        constexpr auto vec_desc    = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };

        using scalar_mdg = tyvi::mdgrid<scalar_desc, std::dextents<std::size_t, 3>>;
        using vec_mdg    = tyvi::mdgrid<vec_desc, std::dextents<std::size_t, 3>>;

        auto scalar_grid = scalar_mdg(8, 4, 6);
        auto vec_grid    = vec_mdg(8, 4, 6);

        auto w1a = tyvi::mdgrid_work{};
        auto w1b = tyvi::mdgrid_work{};

        {
            const auto smds_scalar = scalar_grid.staging_mds();
            for (const auto idx : tyvi::sstd::index_space(smds_scalar)) {
                smds_scalar[idx][] = static_cast<int>(idx[0]);
            }
        }
        auto w2a = w1a.sync_from_staging(scalar_grid);

        {
            const auto smds_vec = vec_grid.staging_mds();
            for (const auto idx : tyvi::sstd::index_space(smds_vec)) {
                for (const auto jdx : tyvi::sstd::index_space(smds_vec[idx])) {
                    smds_vec[idx][jdx] = static_cast<int>(jdx[0]);
                }
            }
        }
        auto w2b = w1b.sync_from_staging(vec_grid);

        auto w3 = tyvi::when_all(w2a, w2b);

        auto kernelA = [scalar_mds = scalar_grid.mds(), vec_mds = vec_grid.mds()](const auto& idx) {
            for (const auto jdx : tyvi::sstd::index_space(vec_mds[idx])) {
                vec_mds[idx][jdx] = vec_mds[idx][jdx] * scalar_mds[idx][];
            }
        };

        auto w4 = w3.for_each_index(vec_grid, std::move(kernelA));

        auto kernelB = [scalar_mds = scalar_grid.mds(), vec_mds = vec_grid.mds()](const auto& idx,
                                                                                  const auto& jdx) {
            vec_mds[idx][jdx] = vec_mds[idx][jdx] * scalar_mds[idx][];
        };

        auto w5 = w4.for_each_index(vec_grid, std::move(kernelB));
        auto w6 = w5.sync_to_staging(vec_grid);

        w6.wait();

        {
            const auto smds_scalar = scalar_grid.staging_mds();
            const auto smds_vec    = vec_grid.staging_mds();

            for (const auto idx : tyvi::sstd::index_space(smds_vec)) {
                const auto s = static_cast<int>(idx[0]);
                expect(smds_scalar[idx][] == s);

                for (const auto jdx : tyvi::sstd::index_space(smds_vec[idx])) {
                    expect(smds_vec[idx][jdx] == s * s * static_cast<int>(jdx[0]));
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
