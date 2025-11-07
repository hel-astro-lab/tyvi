#include <boost/ut.hpp> // import boost.ut;

#include <vector>

#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

/// This should trigger asan if the mds points to invalid memory.
constexpr bool
mds_is_usable(const auto mds) {
    auto all_good = true;

    for (const auto idx : tyvi::sstd::index_space(mds)) {
        for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 7; }
    }

    for (const auto idx : tyvi::sstd::index_space(mds)) {
        for (const auto tidx : tyvi::sstd::index_space(mds[idx])) {
            all_good = (all_good and (mds[idx][tidx] == 7));
        }
    }
    return all_good;
}

[[maybe_unused]]
const suite<"mdgrid_buffer resize"> _ = [] {
    using element_type          = int;
    using vec                   = std::vector<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_layout_policy    = std::layout_right;

    "resizing rank 1 grid"_test = [] {
        using grid_extents = std::dextents<std::size_t, 1>;

        using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                          element_extents,
                                                          element_layout_policy,
                                                          grid_extents,
                                                          grid_layout_policy>;

        auto mdg_buff = testing_mdgrid_buffer(100);

        const auto Ebefore = mdg_buff.grid_extents();
        mdg_buff.invalidating_resize(100);
        expect(mdg_buff.grid_extents() == Ebefore);
        expect(mdg_buff.mds().extents() == Ebefore);

        expect(mds_is_usable(mdg_buff.mds()));

        const auto E10 = grid_extents{ 10uz };
        mdg_buff.invalidating_resize(E10);
        expect(mdg_buff.grid_extents() == E10);
        expect(mdg_buff.mds().extents() == E10);

        expect(mds_is_usable(mdg_buff.mds()));
    };

    "resizing rank 3 grid"_test = [] {
        using grid_extents = std::dextents<std::size_t, 3>;

        using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                          element_extents,
                                                          element_layout_policy,
                                                          grid_extents,
                                                          grid_layout_policy>;

        auto mdg_buff = testing_mdgrid_buffer(20, 34, 12);

        const auto Ebefore = mdg_buff.grid_extents();
        mdg_buff.invalidating_resize(20, 34, 12);
        expect(mdg_buff.grid_extents() == Ebefore);
        expect(mdg_buff.mds().extents() == Ebefore);

        expect(mds_is_usable(mdg_buff.mds()));

        const auto newE = grid_extents{ 10uz, 20uz, 24uz };
        mdg_buff.invalidating_resize(newE);
        expect(mdg_buff.grid_extents() == newE);
        expect(mdg_buff.mds().extents() == newE);

        expect(mds_is_usable(mdg_buff.mds()));
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
