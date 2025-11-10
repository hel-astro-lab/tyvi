#include <boost/ut.hpp> // import boost.ut;

#include <utility>

#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_work"> _ = [] {
    "work can split to one"_test = [] {
        expect(nothrow([] {
            auto w1   = tyvi::mdgrid_work{};
            auto [w2] = w1.split<1>();

            tyvi::when_all(w1, w2);
        }));
    };

    "work can split to many"_test = [] {
        expect(nothrow([] {
            auto w1           = tyvi::mdgrid_work{};
            auto [w2, w3, w4] = w1.split<3>();

            tyvi::when_all(w1, w2, w3, w4);
        }));
    };

    "work can split multiple times"_test = [] {
        expect(nothrow([] {
            auto w1         = tyvi::mdgrid_work{};
            auto [w2a, w2b] = w1.split<2>();
            auto [w3a, w3b] = w2a.split<2>();
            auto [w3c, w3d] = w2b.split<2>();

            tyvi::when_all(w1, w2a, w2b, w3a, w3b, w3c, w3d);
        }));
    };

    "work can be returned from a function"_test = [] {
        expect(nothrow([] {
            [] {
                auto w4 = tyvi::mdgrid_work{};
                w4.wait();

                auto w3 = tyvi::mdgrid_work{};
                w3.wait();

                auto w2 = [] {
                    auto w1 = tyvi::mdgrid_work{};
                    tyvi::when_all(w1);
                    w1.wait();
                    return w1;
                }();

                w2.wait();
                return w2;
            }()
                .wait();
        }));
    };

    "work can be moved out of scope"_test = [] {
        expect(nothrow([] {
            auto w1 = tyvi::mdgrid_work{};
            {
                auto w2 = tyvi::mdgrid_work{};
                w1      = std::move(w2);
            }
            w1.wait();
        }));
    };

    "work can be moved constructed"_test = [] {
        expect(nothrow([] {
            auto w1 = tyvi::mdgrid_work{};
            auto w2{ std::move(w1) };
            w2.wait();
        }));
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
