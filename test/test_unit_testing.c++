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
main() {
    "sum"_test = [] {
        expect(sum(0) == 0_i);
        expect(sum(1, 2) == 3_i);
        expect(sum(1, 2) > 0_i and 4_i == sum(2, 2));
    };

    "subfunctions"_test = [] { call_expect_with(true); };
}
