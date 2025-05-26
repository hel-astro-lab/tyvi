#include <boost/ut.hpp> // import boost.ut;

#include "tyvi/tyvi_hello_world.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"unit testing"> _ = [] {
    "tyvi hello world"_test = [] {
        // Expects hello world to print true.
        expect(tyvi::hello_world());
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
