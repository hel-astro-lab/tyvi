#include <boost/ut.hpp> // import boost.ut;

#include <string>
#include <variant>

#include "tyvi/actions_ast.h"
#include "tyvi/actions_eval.h"
#include "tyvi/actions_list.h"
#include "tyvi/execution.h"

namespace {
using namespace boost::ut;
namespace ta = tyvi::actions;
namespace te = tyvi::exec;
using namespace std::literals;
using ti = ta::intrinsic;

[[maybe_unused]]
const suite<"actions_ast"> _ = [] {
    enum class action : std::uint8_t { append, foobar };

    static constexpr auto make_concatter = [](std::string& str) {
        return [&](ta::sexpr args) -> ta::sexpr_sender {
            return te::just(std::move(args)) | te::then([&](const ta::sexpr& s) -> ta::sexpr {
                       const auto arg = std::get<ta::cons>(s).car();
                       str += ta::atom_cast<std::string>(std::get<ta::atom>(arg)).value();
                       return ta::null;
                   });
        };
    };

    static constexpr auto make_static_concatter = [](std::string& str, const std::string& x) {
        return [&str, x](const ta::sexpr&) -> ta::sexpr_sender {
            return te::just() | te::then([&str, x] {
                       str += x;
                       return ta::null;
                   });
        };
    };

    "symbol lookup"_test = [] {
        const auto env = ta::list(ta::cons(action::foobar, "foobar"s));

        auto snd       = ta::eval<action>(action::foobar, env);
        const auto val = tyvi::this_thread::sync_wait(std::move(snd));
        expect(val == ta::sexpr{ "foobar"s });
    };

    "one argument procedure invocation"_test = [] {
        auto str = std::string{};

        const auto env = ta::list(ta::cons(action::append, ta::procedure(make_concatter(str))));
        const auto src = ta::list(action::append, "foo"s);

        auto snd = ta::eval<action>(src, env);

        tyvi::this_thread::sync_wait(std::move(snd));
        expect(str == "foo");
        tyvi::this_thread::sync_wait(ta::eval<action>(src, env));
        expect(str == "foofoo");
    };

    "zero argument procedure invocation"_test = [] {
        auto str = std::string{};

        const auto env = ta::list(
            ta::cons(action::foobar, ta::procedure(make_static_concatter(str, "foobar"s))));

        const auto src = ta::list(action::foobar);

        auto snd = ta::eval<action>(src, env);

        tyvi::this_thread::sync_wait(std::move(snd));
        expect(str == "foobar") << str;
        tyvi::this_thread::sync_wait(ta::eval<action>(src, env));
        expect(str == "foobarfoobar") << str;
    };

    "procedure literal"_test = [] {
        auto str = std::string{};
        auto f   = make_concatter(str);

        const auto src = ta::list(ta::procedure(f), "foo"s);

        tyvi::this_thread::sync_wait(ta::eval(src, ta::list()));
        expect(str == "foo");
        tyvi::this_thread::sync_wait(ta::eval(src, ta::list()));
        expect(str == "foofoo");
    };

    "quote eval"_test = [] {
        const auto x = tyvi::this_thread::sync_wait(ta::eval(ta::list(ti::quote, 42), ta::list()));
        const auto y = tyvi::this_thread::sync_wait(
            ta::eval(ta::list(ti::quote, ta::cons(42, 43)), ta::list()));
        expect(std::get<ta::atom>(x) == ta::atom{ 42 });
        expect(std::get<ta::cons>(y) == ta::cons(42, 43));
    };

    "simple car eval"_test = [] {
        const auto x = tyvi::this_thread::sync_wait(
            ta::eval(ta::list(ti::car, ta::list(ti::quote, ta::cons(42, 43))), ta::list()));
        expect(std::get<ta::atom>(x) == ta::atom{ 42 });
    };

    "simple cdr eval"_test = [] {
        const auto x = tyvi::this_thread::sync_wait(
            ta::eval(ta::list(ti::cdr, ta::list(ti::quote, ta::cons(42, 43))), ta::list()));
        expect(std::get<ta::atom>(x) == ta::atom{ 43 });
    };

    "cdr, car and quote eval"_test = [] {
        auto str = std::string{};

        const auto env = ta::list(ta::cons(action::append, ta::procedure(make_concatter(str))));

        auto list      = ta::list(ti::quote, ta::list("foo"s, "bar"s, "xyz"s));
        auto bar       = ta::list(ti::car, ta::list(ti::cdr, std::move(list)));
        const auto src = ta::list(action::append, std::move(bar));

        auto snd = ta::eval<action>(src, env);

        tyvi::this_thread::sync_wait(std::move(snd));
        expect(str == "bar");
        tyvi::this_thread::sync_wait(ta::eval<action>(src, env));
        expect(str == "barbar");
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}
