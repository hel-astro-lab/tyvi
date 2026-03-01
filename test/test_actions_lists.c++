#include "constant_testing.h"
#include <boost/ut.hpp> // import boost.ut;

#include <ranges>
#include <vector>

#include "tyvi/actions_ast.h"
#include "tyvi/actions_list.h"
#include "tyvi/execution.h"

namespace {
using namespace boost::ut;
namespace ta = tyvi::actions;
namespace te = tyvi::exec;
using namespace std::literals;

[[maybe_unused]]
const suite<"actions_lists"> _ = [] {
    "C++: empty list creation"_test = [] {
        tyvi::constant_testing(
            [](auto& tester) static consteval { tester.expect(ta::list() == ta::sexpr{}); });
    };

    "C++: one long list creation"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            tester.expect(ta::list(42) == ta::sexpr{ ta::cons(42, ta::null) });
            tester.expect(ta::list("foo"s) == ta::sexpr{ ta::cons("foo"s, ta::null) });
            tester.expect(ta::list(ta::null) == ta::sexpr{ ta::cons(ta::null, ta::null) });
            tester.expect(ta::list(ta::cons(1, 2))
                          == ta::sexpr{ ta::cons(ta::cons(1, 2), ta::null) });
        });
    };
    "C++: two long list creation"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            tester.expect(ta::list(1, 2) == ta::sexpr{ ta::cons(1, ta::cons(2, ta::null)) });
            tester.expect(ta::list("foo"s, "bar"s)
                          == ta::sexpr{ ta::cons("foo"s, ta::cons("bar"s, ta::null)) });
        });
    };

    "C++: list is constructible from range of atoms"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto empty = ta::list(std::from_range_t{}, std::vector<int>{});
            const auto one   = ta::list(std::from_range_t{}, std::vector<int>{ 1 });
            const auto two   = ta::list(std::from_range_t{}, std::vector<int>{ 1, 2 });
            const auto three = ta::list(std::from_range_t{}, std::vector<int>{ 1, 2, 3 });

            tester.expect(empty == ta::sexpr{ ta::list() });
            tester.expect(one == ta::sexpr{ ta::list(1) });
            tester.expect(two == ta::sexpr{ ta::list(1, 2) });
            tester.expect(three == ta::sexpr{ ta::list(1, 2, 3) });

            const auto s = ta::list(std::from_range_t{}, std::vector<ta::atom>{ 1, "2"s, '3' });
            tester.expect(s == ta::sexpr{ ta::list(1, "2"s, '3') });
            tester.expect(s != ta::sexpr{ ta::list(1, 2, 3) });
        });
    };

    "C++: is_list"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            tester.expect(std::visit(ta::is_list, ta::list()));
            tester.expect(std::visit(ta::is_list, ta::list(1)));
            tester.expect(std::visit(ta::is_list, ta::list(1, 2)));

            tester.expect(not std::visit(ta::is_list, ta::sexpr{ ta::cons(1, 2) }));
            tester.expect(not std::visit(ta::is_list, ta::sexpr{ ta::cons(1, ta::cons(2, 3)) }));
            tester.expect(std::visit(ta::is_list, ta::sexpr{ ta::cons(1, ta::cons(2, ta::null)) }));
        });
    };

    "C++: list append"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto foo   = ta::list(1, 2);
            const auto bar   = ta::list(3);
            const auto empty = ta::list();

            // No arguments:
            tester.expect(std::visit(ta::list_append) == ta::list());
            // One argument:
            tester.expect(std::visit(ta::list_append, ta::sexpr{ ta::null })
                          == ta::sexpr{ ta::null });
            tester.expect(std::visit(ta::list_append, ta::sexpr{ 42 }) == ta::sexpr{ 42 });
            tester.expect(std::visit(ta::list_append, foo) == ta::list(1, 2));
            // Two lists:
            tester.expect(std::visit(ta::list_append, foo, bar) == ta::list(1, 2, 3));
            tester.expect(std::visit(ta::list_append, foo, empty) == ta::list(1, 2));
            tester.expect(std::visit(ta::list_append, empty, bar) == ta::list(3));
            // List and non-lists:
            tester.expect(std::visit(ta::list_append, bar, ta::sexpr{ 42 })
                          == ta::sexpr{ ta::cons(3, 42) });

            // See comment above list_append_closure, why this is not possible.
            // Multiple lists:
            // tester.expect(std::visit(ta::list_append, foo, empty, bar) == ta::list(1, 2, 3));
        });
    };

    "C++: list_view"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            tester.expect(std::ranges::empty(ta::list_view(ta::list())));
            tester.expect(std::ranges::empty(ta::list_view(ta::null)));

            const auto list      = ta::list(1, ta::cons(2, 3), ta::null);
            const auto list_view = ta::list_view(list);

            auto it = std::ranges::begin(list_view);

            tester.expect(std::get<ta::atom>(*it++) == ta::atom(1));
            tester.expect(std::get<ta::cons>(*it++) == ta::cons(2, 3));
            tester.expect(std::holds_alternative<ta::null_type>(*it++));
            tester.expect(it == std::ranges::end(list_view));
        });
    };

    "C++: assoc"_test = [] {
        tyvi::constant_testing([](auto& tester) static consteval {
            const auto list = ta::list(ta::cons("foo"s, "bar"s), ta::cons(2, 3));
            tester.expect(not std::visit(ta::assoc("bar"s), list).has_value());
            tester.expect(not std::visit(ta::assoc(3), list).has_value());
            tester.expect(ta::cons("foo"s, "bar"s) == std::visit(ta::assoc("foo"s), list).value());
            tester.expect(ta::cons(2, 3) == std::visit(ta::assoc(2), list).value());

            tester.expect(not std::visit(ta::assoc(2), ta::list()).has_value());
        });
    };

    "C++: map"_test = [] {
        auto x2 = [](const ta::sexpr& s) -> ta::sexpr_sender {
            return te::just(s) | te::then([](ta::sexpr&& x) -> ta::sexpr {
                       auto op =
                           tyvi::sstd::overloaded{ [](const ta::atom& x) -> ta::sexpr {
                                                      if (const auto str =
                                                              ta::atom_cast<std::string>(x)) {
                                                          return str.value() + str.value();
                                                      }
                                                      if (const auto i = ta::atom_cast<int>(x)) {
                                                          return 2 * i.value();
                                                      }
                                                      expect(false);
                                                      return {};
                                                  },
                                                   [](const auto&) -> ta::sexpr {
                                                       expect(false);
                                                       return {};
                                                   } };
                       return std::visit(op, std::move(x));
                   });
        };

        auto snd          = ta::map(x2, ta::list("foo"s, "bar"s, 2, 3));
        const auto mapped = tyvi::this_thread::sync_wait(std::move(snd));
        expect(mapped == ta::list("foofoo"s, "barbar"s, 4, 6));

        const auto mapped_empty = tyvi::this_thread::sync_wait(ta::map(x2, ta::list()));
        expect(mapped_empty == ta::list());
    };
};
} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
