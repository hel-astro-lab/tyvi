#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <tuple>
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
        auto w = tyvi::mdgrid_work{};
        w.wait();

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

        auto w = tyvi::mdgrid_work{};
        w.sync_from_staging(grid).for_each(grid, [](const auto& M) {
            for (const auto Midx : tyvi::sstd::index_space(M)) { M[Midx] = M[Midx] * 3 * 2; }
        });

        w.sync_to_staging(grid).wait();

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

        auto w          = tyvi::mdgrid_work{};
        auto [w1a, w1b] = w.split<2>();

        {
            const auto smds_scalar = scalar_grid.staging_mds();
            for (const auto idx : tyvi::sstd::index_space(smds_scalar)) {
                smds_scalar[idx][] = static_cast<int>(idx[0]);
            }
        }
        w1a.sync_from_staging(scalar_grid);

        {
            const auto smds_vec = vec_grid.staging_mds();
            for (const auto idx : tyvi::sstd::index_space(smds_vec)) {
                for (const auto jdx : tyvi::sstd::index_space(smds_vec[idx])) {
                    smds_vec[idx][jdx] = static_cast<int>(jdx[0]);
                }
            }
        }
        w1b.sync_from_staging(vec_grid);

        tyvi::when_all(w1a, w1b);

        auto kernelA = [TYVI_CMDS(vec_grid, scalar_grid)](const auto& idx) {
            for (const auto jdx : tyvi::sstd::index_space(vec_grid_mds[idx])) {
                vec_grid_mds[idx][jdx] = vec_grid_mds[idx][jdx] * scalar_grid_mds[idx][];
            }
        };

        w1a.for_each_index(vec_grid, std::move(kernelA));

        auto kernelB = [TYVI_CMDS(vec_grid, scalar_grid)](const auto& idx, const auto& jdx) {
            vec_grid_mds[idx][jdx] = vec_grid_mds[idx][jdx] * scalar_grid_mds[idx][];
        };

        w1a.for_each_index(vec_grid, std::move(kernelB)).sync_to_staging(vec_grid).wait();

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

    "mdgrid for each over submdspan 1"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(3, 4, 5);

        auto staging_mds_A = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_A)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_A[idx])) {
                staging_mds_A[idx][Midx] = 7;
            }
        }

        auto w = tyvi::mdgrid_work{};
        w.sync_from_staging(grid);

        const auto gmds = grid.mds();
        const auto sub_gmds =
            std::submdspan(gmds,
                           std::full_extent,
                           std::tuple{ 1, 3 },
                           std::strided_slice{ .offset = 0, .extent = 5, .stride = 2 });

        w.for_each_index(
             sub_gmds,
             [sub_gmds](const auto& idx, const auto& tidx) { sub_gmds[idx][tidx] = 42; })
            .sync_to_staging(grid)
            .wait();

        auto staging_mds_B = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_B)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_B[idx])) {
                const auto ok1 = idx[1] == 1 or idx[1] == 2;
                const auto ok2 = idx[2] == 0 or idx[2] == 2 or idx[2] == 4;

                if (ok1 and ok2) {
                    expect(staging_mds_B[idx][Midx] == 42)
                        << "expected " << 42 << ", got " << staging_mds_B[idx][Midx];
                } else {
                    expect(staging_mds_B[idx][Midx] == 7)
                        << "expected " << 2 << ", got " << staging_mds_B[idx][Midx];
                }
            }
        }
    };

    "mdgrid for each over submdspan 2"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 1, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(4, 4, 4);

        auto staging_mds_A = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_A)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_A[idx])) {
                staging_mds_A[idx][Midx] = 7;
            }
        }

        auto w = tyvi::mdgrid_work{};
        w.sync_from_staging(grid);

        const auto gmds     = grid.mds();
        const auto di       = std::tuple{ 2, 4 };
        const auto dj       = std::tuple{ 1, 3 };
        const auto sub_gmds = std::submdspan(gmds, di, dj, dj);

        w.for_each_index(sub_gmds, [sub_gmds](const auto& idx) {
            sub_gmds[idx][0] = 42;
            sub_gmds[idx][1] = 43;
            sub_gmds[idx][2] = 44;
        });
        w.sync_to_staging(grid).wait();

        auto staging_mds_B = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_B)) {
            const auto [i, j, k]  = idx;
            const auto exterior_A = i < 2 or j == 0 or k == 0;
            const auto exterior_B = j == 3 or k == 3;
            if (exterior_A or exterior_B) {
                expect(staging_mds_B[idx][0] == 7);
                expect(staging_mds_B[idx][1] == 7);
                expect(staging_mds_B[idx][2] == 7);
            } else {
                expect(staging_mds_B[idx][0] == 42);
                expect(staging_mds_B[idx][1] == 43);
                expect(staging_mds_B[idx][2] == 44);
            }
        }
    };

    "mdgrid for each over submdspan 3"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto grid = mdg(7, 9, 2);

        auto staging_mds_A = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_A)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_A[idx])) {
                staging_mds_A[idx][Midx] = 7;
            }
        }

        auto w = tyvi::mdgrid_work{};
        w.sync_from_staging(grid);

        const auto gmds = grid.mds();
        const auto sub_gmds =
            std::submdspan(gmds,
                           std::tuple{ 3, 6 },
                           std::strided_slice{ .offset = 2, .extent = 7, .stride = 2 },
                           std::full_extent);

        w.for_each_index(sub_gmds, [sub_gmds](const auto& idx, const auto& tidx) {
            sub_gmds[idx][tidx] = 42;
        });
        w.sync_to_staging(grid);
        w.wait();

        auto staging_mds_B = grid.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_B)) {
            const auto ok0 = idx[0] == 3 or idx[0] == 4 or idx[0] == 5;
            const auto ok1 = idx[1] == 2 or idx[1] == 4 or idx[1] == 6 or idx[1] == 8;

            for (const auto Midx : tyvi::sstd::index_space(staging_mds_B[idx])) {
                if (ok0 and ok1) {
                    expect(staging_mds_B[idx][Midx] == 42)
                        << "expected " << 42 << ", got " << staging_mds_B[idx][Midx];
                } else {
                    expect(staging_mds_B[idx][Midx] == 7)
                        << "expected " << 2 << ", got " << staging_mds_B[idx][Midx];
                }
            }
        }
    };

    "mdgrid getting and setting underlying buffer"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;

        auto gridA = mdg(7, 9, 2);
        auto gridB = mdg(7, 9, 2);

        auto staging_mds_A = gridA.staging_mds();
        for (const auto idx : tyvi::sstd::index_space(staging_mds_A)) {
            for (const auto Midx : tyvi::sstd::index_space(staging_mds_A[idx])) {
                staging_mds_A[idx][Midx] = 7;
            }
        }

        namespace rn = std::ranges;
        expect(not rn::equal(gridA.staging_span(), gridB.staging_span()));
        gridB.set_underlying_staging_buffer(gridA.underlying_staging_buffer());
        expect(rn::equal(gridA.staging_span(), gridB.staging_span()));

        tyvi::mdgrid_work{}.sync_from_staging(gridA).wait();

        expect(gridA.underlying_buffer() != gridB.underlying_buffer());
        gridB.set_underlying_buffer(gridA.underlying_buffer());
        expect(gridA.underlying_buffer() == gridB.underlying_buffer());
    };

    "mdgrid invalidating_resize"_test = [] {
        constexpr auto elem_desc = tyvi::mdgrid_element_descriptor<int>{ .rank = 2, .dim = 3 };

        using mdg = tyvi::mdgrid<elem_desc, std::dextents<std::size_t, 3>>;
        using E   = mdg::grid_extents_type;

        auto grid = mdg(7, 9, 2);
        grid.invalidating_resize(1, 2, 3);
        expect(grid.extents() == E{ 1, 2, 3 });

        const auto e = E{ 4, 2, 1 };
        grid.invalidating_resize(e);
        expect(grid.mds().extents() == e);
        expect(grid.staging_mds().extents() == e);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
