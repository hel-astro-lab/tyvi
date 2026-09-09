#include <boost/ut.hpp> // import boost.ut;

#include <string>
#include <version>

#include "tyvi/actions_ast.h"
#include "tyvi/actions_format.h"
#include "tyvi/actions_list.h"

namespace {
using namespace boost::ut;
namespace ta = tyvi::actions;

void
check(const auto& x, const auto expected) {
    const auto f = std::format("{}", x);
    expect(f == expected) << std::format("expected: '{}'\ngot: '{}'", expected, f);
}

const auto s = [] {
    "null -> ()"_test = [] {
        check(ta::null, "()");
        check(ta::sexpr{ ta::null }, "()");
    };

    "atom<int> -> std::format(\"{}\", int)"_test = [] {
        check(ta::atom{ 42 }, "42");
        check(ta::sexpr{ ta::atom{ 42 } }, "42");
    };

    "cons(atom<int>, atom<float>) -> std::format(\"({} . {})\", int, float)"_test = [] {
        check(ta::cons(42, 4.2), "(42 . 4.2)");
        check(ta::sexpr{ ta::cons(42, 4.2) }, "(42 . 4.2)");
    };

    "nested cons"_test = [] {
        check(ta::list(42, ta::cons("foo", "bar"), false), "(42 . ((foo . bar) . (false . ())))");
    };
};
} // namespace

int
main(int argc, const char** argv) {
    [[maybe_unused]]
    const suite<"actions_format"> _ = s;
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
