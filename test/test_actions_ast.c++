#include "constant_testing.h"
#include <boost/ut.hpp> // import boost.ut;

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "tyvi/actions.h"

// NOLINTBEGIN{misc-reduntant-expression}

namespace {
using namespace boost::ut;
namespace ta = tyvi::actions;
using namespace std::literals;

[[maybe_unused]]
const suite<"actions_ast"> _ = [] {
    "int atom"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto x = ta::atom{ 10 };
            const auto y = ta::atom{ 42 };
            tester.expect(ta::atom_cast<int>(x).value() == 10);
            tester.expect(ta::atom_cast<int>(y).value() == 42);
            tester.expect(x == x);
            tester.expect(y == y);
            tester.expect(x != y);
            tester.expect(y != x);
        });
    };
    "string atom"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto x = ta::atom{ std::string{ "foo" } };
            const auto y = ta::atom{ std::string{ "bar" } };
            tester.expect(ta::atom_cast<std::string>(x).value() == "foo");
            tester.expect(ta::atom_cast<std::string>(y).value() == "bar");
            tester.expect(x == x);
            tester.expect(y == y);
            tester.expect(x != y);
            tester.expect(y != x);
        });
    };

    "int atoms are moveable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            auto a = ta::atom{ 42 };
            auto b = ta::atom{ 42 };
            tester.expect(a == b);

            auto c = std::move(a);
            tester.expect(c == c);
            tester.expect(b == c);
            tester.expect(c == b);

            a = std::move(b);
            tester.expect(a == a);
            tester.expect(a == c);
            tester.expect(c == a);

            // Not testing self-move because it is most likely ub.
        });
    };

    "int atoms are copyable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a = ta::atom{ 42 };
            const auto A = ta::atom{ a };

            tester.expect(a == A);
            tester.expect(A == a);

            auto B = ta::atom{ 43 };
            tester.expect(a != B);
            tester.expect(B != a);
            B = a;
            tester.expect(a == B);
            tester.expect(B == a);

            // These are assumed to work with GCC and clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
            B = B;
#pragma GCC diagnostic pop
            tester.expect(B == B);
            tester.expect(a == B);
            tester.expect(B == a);
        });
    };

    "string atoms are movable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            auto a = ta::atom{ std::string{ "foo" } };
            auto b = ta::atom{ std::string{ "foo" } };
            tester.expect(a == b);
            tester.expect(b == a);

            auto c = std::move(a);
            tester.expect(c == c);
            tester.expect(b == c);
            tester.expect(c == b);

            a = std::move(b);
            tester.expect(a == a);
            tester.expect(a == c);
            tester.expect(c == a);

            // Not testing self-move because it is most likely ub.
        });
    };

    "string atoms are copyable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a = ta::atom{ std::string{ "foo" } };
            const auto A = ta::atom{ a };

            tester.expect(a == A);
            tester.expect(A == a);

            auto B = ta::atom{ std::string{ "bar" } };
            tester.expect(a != B);
            tester.expect(B != a);
            B = a;
            tester.expect(a == B);
            tester.expect(B == a);

            // These are assumed to work with GCC and clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
            B = B;
#pragma GCC diagnostic pop
            tester.expect(B == B);
            tester.expect(a == B);
            tester.expect(B == a);
        });
    };

    "empty cons"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto cons = ta::cons{};

            tester.expect(cons == cons);

            const auto op = tyvi::sstd::overloaded{ [](std::monostate) { return 42; },
                                                    [](auto&&) { return 1; } };

            const auto icar = std::visit(op, cons.car());
            const auto icdr = std::visit(op, cons.cdr());

            tester.expect(icar == 42);
            tester.expect(icdr == 42);
        });
    };

    "car only cons"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a     = ta::make_cons(42); // implicit atom
            const auto b     = ta::make_cons(ta::atom{ 42 });
            const auto empty = ta::make_cons();

            tester.expect(a == a);
            tester.expect(b == b);
            tester.expect(a == b);
            tester.expect(b == a);

            tester.expect(a != empty);
            tester.expect(empty != a);
            tester.expect(b != empty);
            tester.expect(empty != b);

            const auto op = tyvi::sstd::overloaded{ [](const ta::atom& a) {
                                                       return ta::atom_cast<int>(a).value();
                                                   },
                                                    [](std::monostate) { return 1; },
                                                    [](auto&&) { return 2; } };

            const auto a_icar = std::visit(op, a.car());
            const auto a_icdr = std::visit(op, a.cdr());
            const auto b_icar = std::visit(op, b.car());
            const auto b_icdr = std::visit(op, b.cdr());
            tester.expect(a_icar == 42);
            tester.expect(a_icdr == 1);
            tester.expect(b_icar == 42);
            tester.expect(b_icdr == 1);
        });
    };

    "car and cdr in cons"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a = ta::make_cons(42, 43); // implicit atom
            const auto b = ta::make_cons(42, ta::atom{ 43 });
            const auto x = ta::cons{};
            const auto y = ta::make_cons('f');

            tester.expect(a == a);
            tester.expect(b == b);
            tester.expect(a == b);
            tester.expect(b == a);

            tester.expect(a != x);
            tester.expect(x != a);
            tester.expect(b != x);
            tester.expect(x != b);

            tester.expect(a != y);
            tester.expect(y != a);
            tester.expect(b != y);
            tester.expect(y != b);

            const auto op = tyvi::sstd::overloaded{ [](const ta::atom& a) {
                                                       return ta::atom_cast<int>(a).value();
                                                   },
                                                    [](auto&&) { return 1; } };

            const auto a_icar = std::visit(op, a.car());
            const auto a_icdr = std::visit(op, a.cdr());
            const auto b_icar = std::visit(op, b.car());
            const auto b_icdr = std::visit(op, b.cdr());
            tester.expect(a_icar == 42);
            tester.expect(a_icdr == 43);
            tester.expect(b_icar == 42);
            tester.expect(b_icdr == 43);
        });
    };

    "nested cons equality comparison"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a = ta::make_cons(1, ta::make_cons(ta::make_cons(2, 3), 4));
            const auto b = ta::make_cons(2, ta::make_cons(ta::make_cons(2, 3), 4));
            const auto c = ta::make_cons(1, ta::make_cons(ta::make_cons('3', 3), 4));
            const auto d = ta::make_cons(1, ta::make_cons(ta::make_cons(2, 4), 4));
            const auto e = ta::make_cons(1, ta::make_cons(ta::make_cons(2, 3), '5'));
            const auto f = ta::make_cons();
            const auto g = ta::make_cons(ta::make_cons(), ta::make_cons());

            tester.expect(a == a);
            tester.expect(a != b);
            tester.expect(a != c);
            tester.expect(a != d);
            tester.expect(a != e);
            tester.expect(a != f);
            tester.expect(a != g);
            tester.expect(b != a);
            tester.expect(c != a);
            tester.expect(d != a);
            tester.expect(e != a);
            tester.expect(f != a);
            tester.expect(g != a);
        });
    };

    "nested cons are moveable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            auto a = ta::make_cons("foo"s, ta::make_cons(ta::make_cons("foo"s, ), 4));
            auto b = ta::make_cons("foo"s, ta::make_cons(ta::make_cons("foo"s, ), 4));
            tester.expect(a == b);

            auto c = std::move(a);
            tester.expect(c == c);
            tester.expect(b == c);
            tester.expect(c == b);

            a = std::move(b);
            tester.expect(a == a);
            tester.expect(a == c);
            tester.expect(c == a);
        });
    };

    "nested cons are copyable"_test = []() {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto a = ta::make_cons("foo"s, ta::make_cons(ta::make_cons("foo"s, ), 4));
            const auto A = a;

            tester.expect(a == A);
            tester.expect(A == a);

            auto B = ta::make_cons("foo"s, ta::make_cons(ta::make_cons("bar"s, ), 4));
            tester.expect(a != B);
            tester.expect(B != a);
            B = a;
            tester.expect(a == B);
            tester.expect(B == a);

            // These are assumed to work with GCC and clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
            B = B;
#pragma GCC diagnostic pop
            tester.expect(B == B);
            tester.expect(a == B);
            tester.expect(B == a);
        });
    };
};

} // namespace

// NOLINTEND{misc-reduntant-expression}

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
