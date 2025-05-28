#include <boost/ut.hpp> // import boost.ut;

#include <array>
#include <ranges>

#include "tyvi/sstd.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"sstd"> _ = [] {
    "constexpr integer pow"_test = [] {
        // Use consteval lambda to make sure integer_pow is usable during compilation.
        constexpr auto buff0 = []() consteval {
            return std::array{ tyvi::sstd::ipow(0, 0),
                               tyvi::sstd::ipow(0, 1),
                               tyvi::sstd::ipow(0, 2),
                               tyvi::sstd::ipow(0, 3) };
        }();
        constexpr auto buff1 = []() consteval {
            return std::array{ tyvi::sstd::ipow(1, 0),
                               tyvi::sstd::ipow(1, 1),
                               tyvi::sstd::ipow(1, 2),
                               tyvi::sstd::ipow(1, 3) };
        }();
        constexpr auto buff2 = []() consteval {
            return std::array{ tyvi::sstd::ipow(2, 0),
                               tyvi::sstd::ipow(2, 1),
                               tyvi::sstd::ipow(2, 2),
                               tyvi::sstd::ipow(2, 3) };
        }();

        expect(buff0[0] == 1);
        expect(buff0[1] == 0);
        expect(buff0[2] == 0);
        expect(buff0[3] == 0);

        expect(buff1[0] == 1);
        expect(buff1[1] == 1);
        expect(buff1[2] == 1);
        expect(buff1[3] == 1);

        expect(buff2[0] == 1);
        expect(buff2[1] == 2);
        expect(buff2[2] == 4);
        expect(buff2[3] == 8);

        expect(tyvi::sstd::ipow(3uz, 3uz) == 27uz);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
