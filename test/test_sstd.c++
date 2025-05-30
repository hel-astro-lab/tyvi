#include <boost/ut.hpp> // import boost.ut;

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>

#include "tyvi/mdspan.h"
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

    "geometric extents"_test = [] {
        using ge0 = tyvi::sstd::geometric_extents<0, 0>;
        using ge1 = tyvi::sstd::geometric_extents<1, 1>;
        using ge2 = tyvi::sstd::geometric_extents<2, 2>;
        using ge3 = tyvi::sstd::geometric_extents<3, 3>;

        expect(std::same_as<ge0, std::extents<std::size_t>>);
        expect(std::same_as<ge1, std::extents<std::size_t, 1>>);
        expect(std::same_as<ge2, std::extents<std::size_t, 2, 2>>);
        expect(std::same_as<ge3, std::extents<std::size_t, 3, 3, 3>>);
    };

    "geometric mdspan"_test = [] {
        const auto buff = std::array{ 1, 2, 3, 4 };
        const auto mds  = tyvi::sstd::geometric_mdspan<const int, 2, 2>(buff.data());

        expect(mds[0, 0] == 1);
        expect(mds[0, 1] == 2);
        expect(mds[1, 0] == 3);
        expect(mds[1, 1] == 4);
    };

    "geometric index space"_test = [] {
        auto buff      = std::array{ 1uz, 2uz, 3uz, 4uz };
        const auto mds = tyvi::sstd::geometric_mdspan<std::size_t, 2, 2>(buff.data());

        expect(mds[0, 0] == 1);
        expect(mds[0, 1] == 2);
        expect(mds[1, 0] == 3);
        expect(mds[1, 1] == 4);

        for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) { mds[idx] = idx[0]; }

        expect(mds[0, 0] == 0);
        expect(mds[0, 1] == 0);
        expect(mds[1, 0] == 1);
        expect(mds[1, 1] == 1);

        for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) { mds[idx] = idx[1]; }

        expect(mds[0, 0] == 0);
        expect(mds[0, 1] == 1);
        expect(mds[1, 0] == 0);
        expect(mds[1, 1] == 1);

        "rank 0 geometric index space"_test = [] {
            expect(1 == std::ranges::distance(tyvi::sstd::geometric_index_space<0, 42>()));
            expect(0 == std::ranges::size(tyvi::sstd::geometric_index_space<0, 42>()[0]));
        };
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
