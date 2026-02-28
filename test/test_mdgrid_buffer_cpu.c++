#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <ranges>

#include "tyvi/dynamic_array_cpu.h"
#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_buffer_cpu"> _ = [] {
    using element_type          = int;
    using vec                   = tyvi::dynamic_array<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_extents          = std::dextents<std::size_t, 3>;
    using grid_layout_policy    = std::layout_right;

    using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                      element_extents,
                                                      element_layout_policy,
                                                      grid_extents,
                                                      grid_layout_policy>;

    "mdgrid_buffer<dynamic_array> is constructible"_test = [] {
        [[maybe_unused]]
        auto _ = testing_mdgrid_buffer(2, 3, 4);
        expect(true);
    };

    "mdgrid_buffer<dynamic_array> mds() is usable"_test = [] {
        auto buff      = testing_mdgrid_buffer(2, 3, 4);
        const auto mds = buff.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 42; }
        }

        const auto cbuff = buff;
        const auto cmds  = cbuff.mds();

        for (const auto idx : tyvi::sstd::index_space(cmds)) {
            for (const auto tidx : tyvi::sstd::index_space(cmds[idx])) {
                expect(cmds[idx][tidx] == 42);
            }
        }
    };

    "mdgrid_buffer<dynamic_array> span() works"_test = [] {
        auto buff      = testing_mdgrid_buffer(4, 5, 6);
        const auto mds = buff.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 100; }
        }

        const auto s = buff.span();
        namespace rn = std::ranges;
        expect(rn::all_of(s, [](const auto x) { return x == 100; }));
    };

    "mdgrid_buffer<dynamic_array> invalidating_resize"_test = [] {
        auto buff  = testing_mdgrid_buffer(4, 5, 6);
        const auto e = grid_extents{ 2, 3, 4 };
        buff.invalidating_resize(e);
        expect(buff.grid_extents() == e);
        expect(buff.mds().extents() == e);
    };

    "mdgrid_buffer<dynamic_array> set_underlying_buffer"_test = [] {
        auto buff_A      = testing_mdgrid_buffer(4, 5, 6);
        const auto mds_A = buff_A.mds();

        for (const auto idx : tyvi::sstd::index_space(mds_A)) {
            for (const auto tidx : tyvi::sstd::index_space(mds_A[idx])) {
                mds_A[idx][tidx] = static_cast<int>(idx[0] * tidx[0]) + 3;
            }
        }

        auto buff_B = testing_mdgrid_buffer(4, 5, 6);

        namespace rn = std::ranges;
        expect(not rn::equal(buff_A.span(), buff_B.span()));

        buff_B.set_underlying_buffer(buff_A.underlying_buffer());

        expect(rn::equal(buff_A.span(), buff_B.span()));
    };

    "mdgrid_buffer<dynamic_array> offers copy of underlying buffer"_test = [] {
        auto buff = testing_mdgrid_buffer(4, 5, 6);

        const auto buf_A = buff.underlying_buffer();

        // Make sure it is a copy:
        expect(buf_A.data() != buff.span().data());

        namespace rn = std::ranges;
        expect(rn::equal(buf_A, buff.span()));
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
