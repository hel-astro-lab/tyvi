#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
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
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
