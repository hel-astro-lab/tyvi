#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>

#include "tyvi/mdgrid.h"

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
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
