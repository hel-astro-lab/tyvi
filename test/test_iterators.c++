#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "tyvi/iterators.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"iterators"> _ = [] {
    // ----- counting_iterator -----

    "counting_iterator dereference"_test = [] {
        auto it = tyvi::counting_iterator<int>(5);
        expect(*it == 5);
    };

    "counting_iterator increment"_test = [] {
        auto it = tyvi::counting_iterator<int>(0);
        ++it;
        expect(*it == 1);
        it++;
        expect(*it == 2);
    };

    "counting_iterator decrement"_test = [] {
        auto it = tyvi::counting_iterator<int>(5);
        --it;
        expect(*it == 4);
    };

    "counting_iterator random access"_test = [] {
        auto it = tyvi::counting_iterator<int>(0);
        expect(it[3] == 3);
        expect(it[10] == 10);

        auto it2 = it + 5;
        expect(*it2 == 5);

        auto it3 = it2 - 2;
        expect(*it3 == 3);
    };

    "counting_iterator difference"_test = [] {
        auto a = tyvi::counting_iterator<int>(3);
        auto b = tyvi::counting_iterator<int>(7);
        expect(b - a == 4);
        expect(a - b == -4);
    };

    "counting_iterator comparison"_test = [] {
        auto a = tyvi::counting_iterator<int>(3);
        auto b = tyvi::counting_iterator<int>(5);
        auto c = tyvi::counting_iterator<int>(3);
        expect(a == c);
        expect(a != b);
        expect(a < b);
    };

    "counting_iterator with size_t"_test = [] {
        auto it = tyvi::counting_iterator<std::size_t>(0uz);
        expect(*it == 0uz);
        auto it2 = it + 100;
        expect(*it2 == 100uz);
        expect(it2 - it == 100);
    };

    "counting_iterator in std::for_each"_test = [] {
        auto begin = tyvi::counting_iterator<int>(0);
        auto end   = tyvi::counting_iterator<int>(5);
        auto sum   = 0;
        std::for_each(begin, end, [&](int x) { sum += x; });
        expect(sum == 10); // 0+1+2+3+4
    };

    // ----- transform_iterator -----

    "make_transform_iterator dereference"_test = [] {
        auto data = std::vector<int>{ 1, 2, 3, 4, 5 };
        auto it   = tyvi::make_transform_iterator(data.begin(), [](int x) { return x * 2; });
        expect(*it == 2);
    };

    "make_transform_iterator increment"_test = [] {
        auto data = std::vector<int>{ 10, 20, 30 };
        auto it   = tyvi::make_transform_iterator(data.begin(), [](int x) { return x + 1; });
        expect(*it == 11);
        ++it;
        expect(*it == 21);
        ++it;
        expect(*it == 31);
    };

    "make_transform_iterator random access"_test = [] {
        auto data = std::vector<int>{ 1, 2, 3, 4, 5 };
        auto it   = tyvi::make_transform_iterator(data.begin(), [](int x) { return x * x; });
        expect(it[0] == 1);
        expect(it[1] == 4);
        expect(it[2] == 9);
        expect(it[4] == 25);
    };

    "make_transform_iterator difference"_test = [] {
        auto data  = std::vector<int>{ 1, 2, 3, 4, 5 };
        auto id    = [](int x) { return x; };
        auto begin = tyvi::make_transform_iterator(data.begin(), id);
        auto end   = tyvi::make_transform_iterator(data.end(), id);
        expect(end - begin == 5);
    };

    "make_transform_iterator with counting_iterator"_test = [] {
        auto begin = tyvi::counting_iterator<int>(0);
        auto sq    = tyvi::make_transform_iterator(begin, [](int x) { return x * x; });
        expect(*sq == 0);
        expect(sq[1] == 1);
        expect(sq[3] == 9);
        expect(sq[5] == 25);
    };

    "make_transform_iterator with std::reduce"_test = [] {
        auto data  = std::vector<int>{ 1, 2, 3, 4, 5 };
        auto sq    = [](int x) { return x * x; };
        auto begin = tyvi::make_transform_iterator(data.begin(), sq);
        auto end   = tyvi::make_transform_iterator(data.end(), sq);
        auto sum   = std::reduce(begin, end, 0);
        expect(sum == 55); // 1+4+9+16+25
    };

    // ----- transform_output_iterator -----

    "make_transform_output_iterator write"_test = [] {
        auto dest = std::vector<int>(3);
        auto it   = tyvi::make_transform_output_iterator(
            dest.begin(), [](int x) { return x * 10; });
        *it = 1;
        ++it;
        *it = 2;
        ++it;
        *it = 3;
        expect(dest[0] == 10);
        expect(dest[1] == 20);
        expect(dest[2] == 30);
    };

    "make_transform_output_iterator difference"_test = [] {
        auto dest  = std::vector<int>(5);
        auto id    = [](int x) { return x; };
        auto begin = tyvi::make_transform_output_iterator(dest.begin(), id);
        auto end   = tyvi::make_transform_output_iterator(dest.end(), id);
        expect(end - begin == 5);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
