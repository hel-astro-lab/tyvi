#include <boost/ut.hpp> // import boost.ut;

#include <concepts>
#include <cstddef>
#include <ranges>
#include <utility>

#include "tyvi/dynamic_array_cpu.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"dynamic_array"> _ = [] {
    "default construction"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        expect(arr.size() == 0uz);
        expect(arr.empty());
        expect(arr.capacity() == 0uz);
    };

    "sized construction"_test = [] {
        auto arr = tyvi::dynamic_array<int>(10);
        expect(arr.size() == 10uz);
        expect(arr.capacity() >= 10uz);
        expect(not arr.empty());
    };

    "push_back"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        expect(arr.size() == 3uz);
        expect(arr[0] == 1);
        expect(arr[1] == 2);
        expect(arr[2] == 3);
    };

    "push_back triggers growth"_test = [] {
        auto arr               = tyvi::dynamic_array<int>{};
        const auto initial_cap = arr.capacity();
        for (auto i = 0uz; i < initial_cap + 10; ++i) { arr.push_back(static_cast<int>(i)); }
        expect(arr.size() == initial_cap + 10);
        expect(arr.capacity() > initial_cap);
        for (auto i = 0uz; i < arr.size(); ++i) { expect(arr[i] == static_cast<int>(i)); }
    };

    "resize"_test = [] {
        auto arr = tyvi::dynamic_array<int>(5);
        arr.resize(10);
        expect(arr.size() == 10uz);
        arr.resize(3);
        expect(arr.size() == 3uz);
    };

    "reserve"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.reserve(100);
        expect(arr.capacity() >= 100uz);
        expect(arr.size() == 0uz);
    };

    "shrink_to_fit"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.reserve(100);
        arr.push_back(1);
        arr.push_back(2);
        arr.shrink_to_fit();
        expect(arr.capacity() == 2uz);
        expect(arr.size() == 2uz);
        expect(arr[0] == 1);
        expect(arr[1] == 2);
    };

    "clear"_test = [] {
        auto arr = tyvi::dynamic_array<int>(5);
        arr.clear();
        expect(arr.size() == 0uz);
        expect(arr.empty());
    };

    "operator[]"_test = [] {
        auto arr = tyvi::dynamic_array<int>(3);
        arr[0]   = 10;
        arr[1]   = 20;
        arr[2]   = 30;
        expect(arr[0] == 10);
        expect(arr[1] == 20);
        expect(arr[2] == 30);

        const auto& carr = arr;
        expect(carr[0] == 10);
    };

    "data()"_test = [] {
        auto arr = tyvi::dynamic_array<int>(3);
        arr[0]   = 42;
        expect(arr.data() != nullptr);
        expect(*arr.data() == 42);

        const auto& carr = arr;
        expect(carr.data() != nullptr);
    };

    "begin/end iterators"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        auto sum = 0;
        for (auto it = arr.begin(); it != arr.end(); ++it) { sum += *it; }
        expect(sum == 6);
    };

    "const begin/end iterators"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(10);
        arr.push_back(20);

        const auto& carr = arr;
        auto sum         = 0;
        for (auto it = carr.begin(); it != carr.end(); ++it) { sum += *it; }
        expect(sum == 30);
    };

    "cbegin/cend"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(5);
        auto sum = 0;
        for (auto it = arr.cbegin(); it != arr.cend(); ++it) { sum += *it; }
        expect(sum == 5);
    };

    "set_size"_test = [] {
        auto arr = tyvi::dynamic_array<int>(10);
        arr.set_size(5);
        expect(arr.size() == 5uz);
    };

    "copy constructor"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);

        auto arr2 = arr;
        expect(arr2.size() == 2uz);
        expect(arr2[0] == 1);
        expect(arr2[1] == 2);

        // Verify deep copy
        arr2[0] = 99;
        expect(arr[0] == 1);
    };

    "move constructor"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);

        auto arr2 = std::move(arr);
        expect(arr2.size() == 2uz);
        expect(arr2[0] == 1);
        expect(arr2[1] == 2);
    };

    "copy assignment"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);

        auto arr2 = tyvi::dynamic_array<int>{};
        arr2      = arr;
        expect(arr2.size() == 2uz);
        expect(arr2[0] == 1);
        expect(arr2[1] == 2);

        // Verify deep copy
        arr2[0] = 99;
        expect(arr[0] == 1);
    };

    "move assignment"_test = [] {
        auto arr = tyvi::dynamic_array<int>{};
        arr.push_back(1);
        arr.push_back(2);

        auto arr2 = tyvi::dynamic_array<int>{};
        arr2      = std::move(arr);
        expect(arr2.size() == 2uz);
        expect(arr2[0] == 1);
        expect(arr2[1] == 2);
    };

    "satisfies sized_range"_test = [] {
        expect(std::ranges::sized_range<tyvi::dynamic_array<int>>);
    };

    "satisfies contiguous_range"_test = [] {
        expect(std::ranges::contiguous_range<tyvi::dynamic_array<int>>);
    };

    "equality comparison"_test = [] {
        auto a = tyvi::dynamic_array<int>{};
        a.push_back(1);
        a.push_back(2);

        auto b = tyvi::dynamic_array<int>{};
        b.push_back(1);
        b.push_back(2);

        expect(a == b);

        b.push_back(3);
        expect(a != b);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
