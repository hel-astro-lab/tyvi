#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "tyvi/actions_ast.h"

namespace tyvi::actions {

constexpr auto
list() -> sexpr {
    return null;
}

template<typename Head, typename... Tail>
    requires(not std::same_as<std::from_range_t, std::remove_cvref_t<Head>>)
constexpr auto
list(Head&& head, Tail&&... tail) -> sexpr {
    if constexpr (sizeof...(Tail) == 0) {
        return cons(std::forward<Head>(head), null);
    } else {
        return cons(std::forward<Head>(head), std::get<cons>(list(std::forward<Tail>(tail)...)));
    }
}

constexpr auto
list(std::from_range_t, const std::ranges::bidirectional_range auto& range) -> sexpr {
    if (std::ranges::empty(range)) { return null; }

    auto rev = std::views::reverse(range);

    auto current = cons(*std::ranges::begin(rev), null);

    for (const auto& x : rev | std::views::drop(1)) { current = cons(x, std::move(current)); }

    return current;
}

struct is_list_closure {
    // cppcheck-suppress functionConst
    constexpr auto operator()(this const auto& self, const cons& c) -> bool {
        return std::visit(self, c.cdr());
    }
    static constexpr auto operator()(const null_type&) { return true; }
    static constexpr auto operator()(const atom&) { return false; }
};

// IDK why this can not be recursive lambda with deducing this
constexpr auto is_list = is_list_closure{};

[[nodiscard]]
constexpr auto
is_empty(const sexpr& list) -> bool {
    return std::holds_alternative<null_type>(list);
};

/// Default constructed list_iterators represent end.
class [[nodiscard]] list_iterator {
    cons const* ptr_{ nullptr };

  public:
    using difference_type = std::ptrdiff_t;
    using value_type      = sexpr;

    constexpr list_iterator() = default;
    explicit constexpr list_iterator(null_type) {}
    explicit constexpr list_iterator(const cons& list) : ptr_{ &list } {}

    constexpr auto operator*() const -> value_type { return ptr_->car(); };

    constexpr auto operator++() -> list_iterator& {
        if (std::holds_alternative<null_type>(this->ptr_->cdr())) {
            this->ptr_ = nullptr;
        } else {
            this->ptr_ = &std::get<cons>(this->ptr_->cdr());
        }
        return *this;
    }

    constexpr auto operator++(int) -> list_iterator {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr auto operator==(const list_iterator& other) const -> bool = default;
};

static_assert(std::forward_iterator<list_iterator>);

class [[nodiscard]] list_view {
    list_iterator begin_, end_;

  public:
    explicit constexpr list_view(null_type) {}
    explicit constexpr list_view(const cons& x) : begin_{ x } {}
    explicit constexpr list_view(const sexpr& x) {
        auto op = sstd::overloaded{
            [](const auto& x) { return list_view(x); },
            [](const atom&) -> list_view {
                throw std::runtime_error{ "Can not construct list_view from atom sexpr!" };
            }
        };
        *this = std::visit(op, x);
    }

    constexpr auto begin() const -> list_iterator { return this->begin_; }
    static constexpr auto end() -> list_iterator { return {}; }
};

static_assert(std::ranges::forward_range<list_view>);

static constexpr auto
assoc(const atom& key) {
    auto pred = [=](const sexpr& s) {
        const auto op = tyvi::sstd::overloaded{ [&](const cons& c) {
                                                   if (std::holds_alternative<atom>(c.car())) {
                                                       return std::get<atom>(c.car()) == key;
                                                   }
                                                   return false;
                                               },
                                                [](const auto&) { return false; } };
        return std::visit(op, s);
    };

    return sstd::overloaded{ [pred = std::move(pred)](const cons& alist) -> std::optional<cons> {
                                if (not std::visit(is_list, alist.cdr())) {
                                    throw std::runtime_error{ "Assoc called with non-list!" };
                                }
                                const auto v = list_view(alist);
                                const auto x = std::ranges::find_if(v, pred);
                                if (x == std::ranges::end(v)) { return {}; }
                                return std::get<cons>(*x);
                            },
                             [](const null_type&) -> std::optional<cons> { return {}; },
                             [](const auto&) -> std::optional<cons> {
                                 throw std::runtime_error{ "Assoc called with non-list!" };
                             } };
}

[[nodiscard]]
constexpr auto
map(const procedure_like auto& f, const cons& list) -> sexpr_sender {
    if (std::holds_alternative<null_type>(list.cdr())) {
        return std::invoke(f, list.car())
               | exec::then([](sexpr&& x) -> sexpr { return cons(std::move(x), null); });
    }

    if (std::holds_alternative<atom>(list.cdr())) {
        throw std::runtime_error{ "List given to map is not a proper list!" };
    }

    return exec::when_all(std::invoke(f, list.car()), map(f, std::get<cons>(list.cdr())))
           | exec::then([](sexpr&& head, sexpr&& tail) -> sexpr {
                 return cons(std::move(head), std::move(tail));
             });
}

[[nodiscard]]
constexpr auto
map(const procedure_like auto& f, const sexpr& s) -> sexpr_sender {
    auto op = sstd::overloaded{ [](const null_type&) -> sexpr_sender { return exec::just(null); },
                                [&](const cons& c) { return map(f, c); },
                                [](const auto&) -> sexpr_sender {
                                    throw std::runtime_error{ "Can not map non-lists sexpr." };
                                } };
    return std::visit(op, s);
}

// Can not be made to handle arbitrary number of elements,
// as hipcc seems to be unable to compile it:
//
// /.../include/c++/variant:1098:36: note: const variable cannot be emitted on device side due to dynamic initialization
//  1098 |       static constexpr _Array_type _S_vtable
//
// Even if this is not used in device context.
struct list_append_closure {
    template<typename... Tail>
    static constexpr auto operator()(cons lhs, auto rhs /*, Tail&&... tail*/) -> sexpr {
        if (not is_list(lhs)) { throw std::runtime_error{ "Trying to append a non-list." }; }

        auto* ptr = &lhs;
        while (std::holds_alternative<cons>(ptr->cdr())) { ptr = &std::get<cons>(*ptr->cdr_); }
        *ptr->cdr_ = std::move(rhs);

        /* if (sizeof...(tail) == 0) { */
        return lhs;
        /* } else {
            std::visit(list_append_closure{},
                       sexpr{ std::move(lhs) },
                       sexpr{ std::forward<Tail>(tail) }...);
        } */
    }
    static constexpr auto operator()(null_type, const cons& rhs) -> sexpr { return rhs; }
    static constexpr auto operator()(const cons& lhs, null_type) -> sexpr { return lhs; }
    static constexpr auto operator()(auto&&, auto&& /*, auto&&... */) -> sexpr {
        throw std::runtime_error{ "Invalid list_append!" };
    }
    static constexpr auto operator()(auto&& x) -> sexpr { return x; }
    static constexpr auto operator()() -> sexpr { return null; }
};

constexpr auto list_append = list_append_closure{};

} // namespace tyvi::actions
