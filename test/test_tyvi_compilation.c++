#include <boost/ut.hpp> // import boost.ut;

#include <array>

#include "tyvi/mdspan.h"
#include "tyvi/tyvi_hello_world.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"unit testing"> _ = [] {
    "tyvi hello world"_test = [] {
        // Expects hello world to print true.
        expect(tyvi::hello_world());
    };

    "rocthrust is usable"_test = [] { expect(tyvi::hello_thrust()); };

    "mdspan is usable"_test = [] {
        const auto buff = std::array{ 1, 2, 3, 4 };
        const auto mds  = std::mdspan(buff.data(), 2, 2);

        expect(mds[0, 0] == 1);
        expect(mds[0, 1] == 2);
        expect(mds[1, 0] == 3);
        expect(mds[1, 1] == 4);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
