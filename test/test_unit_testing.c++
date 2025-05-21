#include <boost/ut.hpp> // import boost.ut;

constexpr auto
sum(auto... values) {
    return (values + ...);
}

auto
call_expect_with(const bool x) {
    boost::ut::expect(x);
}

boost::ut::suite<"test_unit_testing"> _ = [] {
    using namespace boost::ut;

    "sum"_test = [] {
        expect(sum(0) == 0_i);
        expect(sum(1, 2) == 3_i);
        expect(sum(1, 2) > 0_i and 42_i == sum(40, 2));
    };

    "subfunctions"_test = [] { call_expect_with(true); };
};

int
main() {}
