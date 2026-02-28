#include <boost/ut.hpp>

#include <cstddef>
#include <vector>

#include "tyvi/containers.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"containers"> _ = [] {
    "device_vector sized construction"_test = [] {
        auto v = tyvi::device_vector<int>(10);
        expect(v.size() == 10uz);
    };

    "device_vector iterator-range construction"_test = [] {
        auto src = std::vector<int>{ 1, 2, 3, 4, 5 };
        auto v   = tyvi::device_vector<int>(src.begin(), src.end());
        expect(v.size() == 5uz);
        expect(v[0] == 1);
        expect(v[4] == 5);
    };

    "device_vector push_back and iterate"_test = [] {
        auto v = tyvi::device_vector<float>{};
        v.push_back(1.0f);
        v.push_back(2.0f);
        v.push_back(3.0f);
        expect(v.size() == 3uz);

        auto sum = 0.0f;
        for (auto it = v.begin(); it != v.end(); ++it) { sum += *it; }
        expect(sum > 5.9f);
        expect(sum < 6.1f);
    };

    "host_vector sized construction"_test = [] {
        auto v = tyvi::host_vector<int>(5);
        expect(v.size() == 5uz);
    };

    "host_vector iterator-range construction"_test = [] {
        auto src = std::vector<double>{ 1.0, 2.0, 3.0 };
        auto v   = tyvi::host_vector<double>(src.begin(), src.end());
        expect(v.size() == 3uz);
        expect(v[0] > 0.9);
        expect(v[0] < 1.1);
    };

    "device_vector assign"_test = [] {
        auto v   = tyvi::device_vector<int>(3);
        auto src = std::vector<int>{ 10, 20 };
        v.assign(src.begin(), src.end());
        expect(v.size() == 2uz);
        expect(v[0] == 10);
        expect(v[1] == 20);
    };

    "device_vector copy and move"_test = [] {
        auto v1 = tyvi::device_vector<int>{};
        v1.push_back(42);

        auto v2 = v1;
        expect(v2.size() == 1uz);
        expect(v2[0] == 42);

        auto v3 = std::move(v2);
        expect(v3.size() == 1uz);
        expect(v3[0] == 42);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
