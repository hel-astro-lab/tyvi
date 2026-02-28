#include <boost/ut.hpp>

#include <array>
#include <cstddef>
#include <numeric>
#include <vector>

#include "tyvi/algo.h"
#include "tyvi/containers.h"
#include "tyvi/iterators.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"algo_extended"> _ = [] {
    // ----- sort_by_key -----

    "sort_by_key basic"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(3);
        keys.push_back(1);
        keys.push_back(2);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(30);
        vals.push_back(10);
        vals.push_back(20);

        tyvi::algo::sort_by_key(keys.begin(), keys.end(), vals.begin());

        expect(keys[0] == 1);
        expect(keys[1] == 2);
        expect(keys[2] == 3);
        expect(vals[0] == 10);
        expect(vals[1] == 20);
        expect(vals[2] == 30);
    };

    "sort_by_key already sorted"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(1);
        keys.push_back(2);
        keys.push_back(3);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(10);
        vals.push_back(20);
        vals.push_back(30);

        tyvi::algo::sort_by_key(keys.begin(), keys.end(), vals.begin());

        expect(vals[0] == 10);
        expect(vals[1] == 20);
        expect(vals[2] == 30);
    };

    "sort_by_key empty"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        auto vals = tyvi::device_vector<int>{};
        tyvi::algo::sort_by_key(keys.begin(), keys.end(), vals.begin());
        expect(keys.empty());
    };

    "sort_by_key with duplicate keys"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(2);
        keys.push_back(1);
        keys.push_back(2);
        keys.push_back(1);

        auto vals = tyvi::device_vector<char>{};
        vals.push_back('a');
        vals.push_back('b');
        vals.push_back('c');
        vals.push_back('d');

        tyvi::algo::sort_by_key(keys.begin(), keys.end(), vals.begin());

        expect(keys[0] == 1);
        expect(keys[1] == 1);
        expect(keys[2] == 2);
        expect(keys[3] == 2);
        // Values follow their original keys
        expect((vals[0] == 'b' or vals[0] == 'd'));
        expect((vals[2] == 'a' or vals[2] == 'c'));
    };

    // ----- reduce -----

    "reduce basic"_test = [] {
        auto v = tyvi::device_vector<int>{};
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);

        auto result = tyvi::algo::reduce(v.begin(), v.end(), 0);
        expect(result == 10);
    };

    "reduce empty"_test = [] {
        auto v      = tyvi::device_vector<int>{};
        auto result = tyvi::algo::reduce(v.begin(), v.end(), 0);
        expect(result == 0);
    };

    "reduce with init"_test = [] {
        auto v = tyvi::device_vector<int>{};
        v.push_back(1);
        v.push_back(2);

        auto result = tyvi::algo::reduce(v.begin(), v.end(), 100);
        expect(result == 103);
    };

    "reduce with transform_iterator"_test = [] {
        auto begin = tyvi::counting_iterator<int>(1);
        auto sq    = tyvi::make_transform_iterator(begin, [](int x) { return x * x; });
        auto end   = sq + 4;

        auto result = tyvi::algo::reduce(sq, end, 0);
        expect(result == 30); // 1+4+9+16
    };

    // ----- reduce_by_key -----

    "reduce_by_key basic"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(1);
        keys.push_back(1);
        keys.push_back(2);
        keys.push_back(2);
        keys.push_back(2);
        keys.push_back(3);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(10);
        vals.push_back(20);
        vals.push_back(1);
        vals.push_back(2);
        vals.push_back(3);
        vals.push_back(100);

        auto out_keys = tyvi::device_vector<int>(6);
        auto out_vals = tyvi::device_vector<int>(6);

        auto [key_end, val_end] = tyvi::algo::reduce_by_key(
            keys.begin(), keys.end(),
            vals.begin(),
            out_keys.begin(), out_vals.begin());

        auto n = key_end - out_keys.begin();
        expect(n == 3);

        expect(out_keys[0] == 1);
        expect(out_keys[1] == 2);
        expect(out_keys[2] == 3);
        expect(out_vals[0] == 30);  // 10+20
        expect(out_vals[1] == 6);   // 1+2+3
        expect(out_vals[2] == 100);
    };

    "reduce_by_key single group"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(1);
        keys.push_back(1);
        keys.push_back(1);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(5);
        vals.push_back(10);
        vals.push_back(15);

        auto out_keys = tyvi::device_vector<int>(3);
        auto out_vals = tyvi::device_vector<int>(3);

        auto [key_end, val_end] = tyvi::algo::reduce_by_key(
            keys.begin(), keys.end(),
            vals.begin(),
            out_keys.begin(), out_vals.begin());

        expect(key_end - out_keys.begin() == 1);
        expect(out_keys[0] == 1);
        expect(out_vals[0] == 30);
    };

    "reduce_by_key empty"_test = [] {
        auto keys     = tyvi::device_vector<int>{};
        auto vals     = tyvi::device_vector<int>{};
        auto out_keys = tyvi::device_vector<int>{};
        auto out_vals = tyvi::device_vector<int>{};

        auto [key_end, val_end] = tyvi::algo::reduce_by_key(
            keys.begin(), keys.end(),
            vals.begin(),
            out_keys.begin(), out_vals.begin());

        expect(key_end == out_keys.begin());
    };

    // ----- unique_by_key -----

    "unique_by_key basic"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(1);
        keys.push_back(1);
        keys.push_back(2);
        keys.push_back(2);
        keys.push_back(3);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(10);
        vals.push_back(20);
        vals.push_back(30);
        vals.push_back(40);
        vals.push_back(50);

        auto [key_end, val_end] = tyvi::algo::unique_by_key(
            keys.begin(), keys.end(), vals.begin());

        auto n = key_end - keys.begin();
        expect(n == 3);
        expect(keys[0] == 1);
        expect(keys[1] == 2);
        expect(keys[2] == 3);
        expect(vals[0] == 10);
        expect(vals[1] == 30);
        expect(vals[2] == 50);
    };

    "unique_by_key no duplicates"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(1);
        keys.push_back(2);
        keys.push_back(3);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(10);
        vals.push_back(20);
        vals.push_back(30);

        auto [key_end, val_end] = tyvi::algo::unique_by_key(
            keys.begin(), keys.end(), vals.begin());

        expect(key_end - keys.begin() == 3);
    };

    "unique_by_key empty"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        auto vals = tyvi::device_vector<int>{};

        auto [key_end, val_end] = tyvi::algo::unique_by_key(
            keys.begin(), keys.end(), vals.begin());

        expect(key_end == keys.begin());
    };

    "unique_by_key all same"_test = [] {
        auto keys = tyvi::device_vector<int>{};
        keys.push_back(5);
        keys.push_back(5);
        keys.push_back(5);
        keys.push_back(5);

        auto vals = tyvi::device_vector<int>{};
        vals.push_back(1);
        vals.push_back(2);
        vals.push_back(3);
        vals.push_back(4);

        auto [key_end, val_end] = tyvi::algo::unique_by_key(
            keys.begin(), keys.end(), vals.begin());

        expect(key_end - keys.begin() == 1);
        expect(keys[0] == 5);
        expect(vals[0] == 1);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
