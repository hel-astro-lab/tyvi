#include <boost/ut.hpp> // import boost.ut;

constexpr auto
sum(auto... values) {
    return (values + ...);
}

auto
call_expect_with(const bool x) {
    boost::ut::expect(x);
}

int
main(int argc, const char** argv) {
    using namespace boost::ut;

    try {
        [[maybe_unused]]
        const suite<"unit testing"> suite_name = [] {
            "sum"_test = [] {
                expect(sum(0) == 0_i);
                expect(sum(1, 2) == 3_i);
                expect(sum(1, 2) > 0_i and 4_i == sum(2, 2));
            };
            "subfunctions"_test = [] { call_expect_with(true); };
        };

        return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
    } catch (...) { return 1; }
}
