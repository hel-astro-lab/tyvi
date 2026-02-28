#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <ranges>
#include <vector>

#include "tyvi/algo.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"algo"> _ = [] {
    "for_each"_test = [] {
        auto vec = std::vector<int>{ 1, 2, 3, 4, 5 };
        tyvi::algo::for_each(vec.begin(), vec.end(), [](auto& x) { x *= 2; });
        expect(vec[0] == 2);
        expect(vec[1] == 4);
        expect(vec[2] == 6);
        expect(vec[3] == 8);
        expect(vec[4] == 10);
    };

    "copy"_test = [] {
        const auto src = std::vector<int>{ 1, 2, 3 };
        auto dst       = std::vector<int>(3, 0);
        tyvi::algo::copy(src.begin(), src.end(), dst.begin());
        expect(std::ranges::equal(src, dst));
    };

    "sort"_test = [] {
        auto vec = std::vector<int>{ 5, 3, 1, 4, 2 };
        tyvi::algo::sort(vec.begin(), vec.end());
        expect(std::ranges::is_sorted(vec));
        expect(vec[0] == 1);
        expect(vec[4] == 5);
    };

    "find"_test = [] {
        const auto vec = std::vector<int>{ 10, 20, 30, 40 };
        auto it        = tyvi::algo::find(vec.begin(), vec.end(), 30);
        expect(it != vec.end());
        expect(*it == 30);

        auto it2 = tyvi::algo::find(vec.begin(), vec.end(), 99);
        expect(it2 == vec.end());
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
