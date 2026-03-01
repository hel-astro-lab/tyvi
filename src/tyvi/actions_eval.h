#pragma once

#include <stdexcept>
#include <utility>
#include <variant>

#include "tyvi/actions_ast.h"
#include "tyvi/actions_list.h"
#include "tyvi/execution.h"

namespace tyvi::actions {

static const auto intrinsic_env =
    list(cons(intrinsic::car, procedure([](sexpr s) -> sexpr_sender {
                  return exec::just(std::move(s)) | exec::then([](const sexpr& s) -> sexpr {
                             if (not std::holds_alternative<cons>(s)) {
                                 throw std::runtime_error{ "Car argument is not (one long) list!" };
                             }

                             const auto arg = std::get<cons>(s).car();

                             if (not std::holds_alternative<cons>(arg)) {
                                 throw std::runtime_error{
                                     "Car argument is not of form: ((a, b), null)"
                                 };
                             }
                             return std::get<cons>(arg).car();
                         });
              })),
         cons(intrinsic::cdr, procedure([](sexpr s) -> sexpr_sender {
                  return exec::just(std::move(s)) | exec::then([](const sexpr& s) -> sexpr {
                             if (not std::holds_alternative<cons>(s)) {
                                 throw std::runtime_error{ "Cdr argument is not (one long) list!" };
                             }

                             const auto arg = std::get<cons>(s).car();

                             if (not std::holds_alternative<cons>(arg)) {
                                 throw std::runtime_error{
                                     "Cdr argument is not of form: ((a, b), null)"
                                 };
                             }
                             return std::get<cons>(arg).cdr();
                         });
              })));

template<typename... Symbols>
auto
eval(const sexpr& body, const sexpr& env) -> sexpr_sender {
    auto op = tyvi::sstd::overloaded{
        [&](const atom& x) -> sexpr_sender {
            if (atom_is_of_type<intrinsic, Symbols...>(x)) {
                return exec::just(std::visit(assoc(x), std::visit(list_append, intrinsic_env, env))
                                      .value()
                                      .cdr());
            }
            return exec::just(sexpr{ x }); // Not a symbol type, i.e. it is "built-in type".
        },
        [&](const cons& c) -> sexpr_sender {
            if (std::holds_alternative<atom>(c.car())) {
                if (const auto intr = atom_cast<intrinsic>(std::get<atom>(c.car()))) {
                    switch (intr.value()) {
                        case intrinsic::quote: {
                            if (not std::holds_alternative<cons>(c.cdr())) {
                                throw std::runtime_error{
                                    "Quote argument is not (one long) list!"
                                };
                            }

                            const auto arg = std::get<cons>(c.cdr());

                            if (not std::holds_alternative<null_type>(arg.cdr())) {
                                throw std::runtime_error{
                                    "Quote argument is not of form: (x, null)"
                                };
                            }

                            return exec::just(arg.car());
                        }
                        default: break;
                    }
                }
            }

            auto proc = eval<Symbols...>(c.car(), env);

            auto args = map(
                [&](const sexpr& x) -> sexpr_sender {
                    return exec::just(x, env)
                           | exec::let_value([](const sexpr& y, const sexpr& e) -> sexpr_sender {
                                 return eval<Symbols...>(y, e);
                             });
                },
                c.cdr());

            return exec::when_all(std::move(proc), std::move(args))
                   | exec::let_value([](const sexpr& p, const sexpr& a) -> sexpr_sender {
                         if (not std::holds_alternative<atom>(p)) {
                             throw std::runtime_error{ "Trying to invoke non-procedure!" };
                         }

                         if (const auto f = atom_cast<procedure>(std::get<atom>(p))) {
                             return std::invoke(f.value(), a);
                         }
                         throw std::runtime_error{ "Trying to invoke non-procedure!" };
                     });
        },
        /*
            return eval<Symbols...>(c.car(), env)
                   | exec::let_value([c=std::move(c)](const sexpr& car) -> sexpr_sender {
                     });
        },
        */
        [](null_type) -> sexpr_sender { throw std::runtime_error{ "Trying to eval null!" }; }
    };

    return std::visit(op, body);
}
} // namespace tyvi::actions
