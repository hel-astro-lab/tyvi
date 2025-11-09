#include <boost/ut.hpp> // import boost.ut;

#include "tyvi/mdgrid.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_work"> _ = [] {
    "work can split to one"_test = [] {
        auto w1   = tyvi::mdgrid_work{};
        auto [w2] = w1.split<1>();

        tyvi::when_all(w1, w2);
    };

    "work can split to many"_test = [] {
        auto w1           = tyvi::mdgrid_work{};
        auto [w2, w3, w4] = w1.split<3>();

        tyvi::when_all(w1, w2, w3, w4);
    };

    "work can split multiple times"_test = [] {
        auto w1         = tyvi::mdgrid_work{};
        auto [w2a, w2b] = w1.split<2>();
        auto [w3a, w3b] = w2a.split<2>();
        auto [w3c, w3d] = w2b.split<2>();

        tyvi::when_all(w1, w2a, w2b, w3a, w3b, w3c, w3d);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
