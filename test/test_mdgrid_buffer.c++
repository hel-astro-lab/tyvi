#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <ranges>
#include <type_traits>
#include <vector>

#include <experimental/mdspan>

#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_buffer"> _ = [] {
    using element_type          = int;
    using vec                   = std::vector<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_extents          = std::dextents<std::size_t, 3>;
    using grid_layout_policy    = std::layout_right;

    using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                      element_extents,
                                                      element_layout_policy,
                                                      grid_extents,
                                                      grid_layout_policy>;

    "mdgrid_buffer is constructible"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        [[maybe_unused]]
        auto _ = testing_mdgrid_buffer(grid_mapping);
        expect(true);
    };

    "mdgrid_buffer is copyable and const correct viewable through mdspan"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        [[maybe_unused]]
        auto buff = testing_mdgrid_buffer(grid_mapping);

        const auto mds = buff.mds();

        expect(not std::is_const_v<
               std::remove_reference_t<typename decltype(mds)::element_type::reference>>);

        namespace rv = std::views;
        auto i_space = rv::iota(0uz, 2uz);
        auto j_space = rv::iota(0uz, 3uz);
        auto k_space = rv::iota(0uz, 4uz);

        auto bad_hash = [](const auto i, const auto j, const auto k, const auto idx) {
            return static_cast<element_type>(i + 10 * j + 100 * k + 1000 * idx[0] + 10000 * idx[1]);
        };

        for (const auto [i, j, k] : rv::cartesian_product(i_space, j_space, k_space)) {
            for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) {
                mds[i, j, k][idx] = bad_hash(i, j, k, idx);
            }
        }

        const auto cbuff = buff;
        const auto cmds  = cbuff.mds();

        expect(std::is_const_v<
               std::remove_reference_t<typename decltype(cmds)::element_type::reference>>);

        for (const auto [i, j, k] : rv::cartesian_product(i_space, j_space, k_space)) {
            for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) {
                expect(cmds[i, j, k][idx] == bad_hash(i, j, k, idx));
            }
        }
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
