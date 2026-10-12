// Copyright 2026 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/* Note that actions ast was constexpr capable until downgrade to C++23
   and move to shared_ptr based memory handling. However, all constexpr specifiers
   are left in place in anticipation of C++26. */

#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include "tyvi/execution.h"

namespace tyvi::sstd {

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

} // namespace tyvi::sstd

namespace tyvi::actions {

class cons;
class atom;

using null_type    = std::monostate;
using sexpr        = std::variant<null_type, cons, atom>;
using sexpr_sender = exec::unique_any_sender<sexpr>;
using procedure    = std::function<exec::unique_any_sender<sexpr>(sexpr)>;

// Special procedure types:
using procedure_with_eval = std::function<exec::unique_any_sender<sexpr>(sexpr, procedure)>;

enum class intrinsic : std::uint8_t { car, cdr, quote };

static constexpr auto null = null_type{};

template<typename F>
concept procedure_like =
    std::invocable<F, sexpr> and std::same_as<std::invoke_result_t<F, sexpr>, sexpr_sender>;

template<typename T>
concept sexpr_like = std::same_as<std::decay_t<T>, null_type> or std::same_as<std::decay_t<T>, cons>
                     or std::same_as<std::decay_t<T>, atom> or std::same_as<std::decay_t<T>, sexpr>;

template<typename T>
concept not_sexpr_like = (not sexpr_like<T>);

class [[nodiscard]] atom {
    /// FIXME: refactor to smart pointers
    std::shared_ptr<void> value_ptr_{ nullptr };
    bool (*comparison_op_)(void const*, void const*){ nullptr };
    std::string (*format_op_)(void const*){ nullptr };
    std::type_info const* value_type_info_{ nullptr };

    constexpr atom() = default;

  public:
    // cppcheck-suppress noExplicitConstructor
    constexpr atom(not_sexpr_like auto&& x); // NOLINT{hicpp-explicit-conversions}

    template<typename T>
    friend constexpr auto atom_cast(const atom&) -> std::optional<T>;

    friend constexpr auto operator==(const atom&, const atom&) -> bool;

    template<typename T, typename... U>
    friend constexpr auto atom_is_of_type(const atom&) -> bool;

    [[nodiscard]]
    constexpr std::string format() const;
};

class [[nodiscard]] cons {
    using sexpr_holder = std::shared_ptr<sexpr>;

    // class invariant: car_ and cdr_ always point to valid sexprs.

    static sexpr_holder shared_null;
    sexpr_holder car_{ shared_null };
    sexpr_holder cdr_{ shared_null };

  public:
    template<typename T, typename U>
    constexpr cons(T&& car, U&& cdr);

    /* constexpr */ cons()                                = default;
    /* constexpr */ ~cons()                               = default;
    /* constexpr */ cons(const cons&) noexcept            = default;
    /* constexpr */ cons& operator=(const cons&) noexcept = default;

    // These can not be defaulted, as they would violate the class invariant.
    constexpr cons(cons&&) noexcept;
    constexpr cons& operator=(cons&&) noexcept;

    [[nodiscard]]
    constexpr auto car() const -> const sexpr&;
    [[nodiscard]]
    constexpr auto cdr() const -> const sexpr&;

    /// Deep equality comparison.
    friend constexpr auto operator==(const cons&, const cons&) -> bool;

    /// Appends two lists togethera.
    friend struct list_append_closure;
};

// Implementations:

inline cons::sexpr_holder cons::shared_null = std::make_shared<sexpr>();

constexpr auto
operator==(const atom& lhs, const atom& rhs) -> bool {
    if (lhs.value_ptr_ and rhs.value_ptr_ and (lhs.value_type_info_ == rhs.value_type_info_)) {
        return (*lhs.comparison_op_)(lhs.value_ptr_.get(), rhs.value_ptr_.get());
    }
    return false;
}

template<not_sexpr_like T>
constexpr atom::atom(T&& value)
    : // NOLINTBEGIN{cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay}
      value_ptr_(static_cast<void*>(new std::decay_t<T>{ std::forward<T>(value) }),
                 [](void* const ptr) {
                     // NOLINTBEGIN{cppcoreguidelines-owning-memory}
                     delete static_cast<std::decay_t<T>* const>(ptr);
                     // NOLINTEND{cppcoreguidelines-owning-memory}
                 }),
      // NOLINTEND{cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay}
      comparison_op_{ [](void const* const lhs_ptr, void const* const rhs_ptr) {
          using ptr_type  = std::decay_t<T> const* const;
          const auto& lhs = *static_cast<ptr_type>(lhs_ptr);
          const auto& rhs = *static_cast<ptr_type>(rhs_ptr);
          return lhs == rhs;
      } },
      format_op_{ [](void const* const src_ptr) -> std::string {
          const auto& src = *static_cast<std::decay_t<T> const* const>(src_ptr);
          if constexpr (std::formattable<decltype(src), char>) {
              return std::format("{}", src);
          } else {
              return std::string{ "<atom>" };
          }
      } },
      value_type_info_{ &typeid(std::decay_t<T>) } {}

template<typename T>
constexpr auto
atom_cast(const atom& x) -> std::optional<T> {
    if (x.value_ptr_ and typeid(T) == *x.value_type_info_) {
        return *static_cast<T*>(x.value_ptr_.get());
    }
    return {};
}

template<typename T, typename... U>
[[nodiscard]]
constexpr auto
atom_is_of_type(const atom& x) -> bool {
    if (x.value_ptr_) {
        return *x.value_type_info_ == typeid(T) or ((*x.value_type_info_ == typeid(U)) or ...);
    }
    return false;
}

constexpr std::string
atom::format() const {
    if (this->value_ptr_) { return (*this->format_op_)(this->value_ptr_.get()); }
    throw std::format_error{ "Trying to format empty" };
}

template<typename T, typename U>
constexpr cons::cons(T&& car, U&& cdr)
    : car_{ std::make_shared<sexpr>(std::forward<T>(car)) },
      cdr_{ std::make_shared<sexpr>(std::forward<U>(cdr)) } {}

constexpr auto
cons::car() const -> const sexpr& {
    return *this->car_;
}

constexpr auto
cons::cdr() const -> const sexpr& {
    return *this->cdr_;
}

constexpr auto
operator==(const cons& LHS, const cons& RHS) -> bool {
    const auto op =
        tyvi::sstd::overloaded{ []<sexpr_like T>(const T& l, const T& r) { return l == r; },
                                [](auto&&, auto&&) { return false; } };

    return std::visit(op, LHS.car(), RHS.car()) and std::visit(op, LHS.cdr(), RHS.cdr());
}

constexpr cons::cons(cons&& other) noexcept {
    this->car_ = std::exchange(other.car_, cons::shared_null);
    this->cdr_ = std::exchange(other.cdr_, cons::shared_null);
}

constexpr cons&
cons::operator=(cons&& other) noexcept {
    this->car_ = std::exchange(other.car_, cons::shared_null);
    this->cdr_ = std::exchange(other.cdr_, cons::shared_null);
    return *this;
}

} // namespace tyvi::actions
