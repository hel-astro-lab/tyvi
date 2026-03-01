#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
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
    void* value_ptr_{ nullptr };
    void (*value_deleter_)(void*){ nullptr };
    bool (*comparison_op_)(void const*, void const*){ nullptr };
    void* (*clone_op_)(void const*){ nullptr };
    std::type_info const* value_type_info_{ nullptr };

    [[nodiscard]]
    constexpr auto no_null_members_() const -> bool;
    constexpr void set_null_();

    constexpr atom() = default;

  public:
    // cppcheck-suppress noExplicitConstructor
    constexpr atom(not_sexpr_like auto&& x); // NOLINT{hicpp-explicit-conversions}
    constexpr atom(const atom&);
    constexpr atom(atom&&) noexcept;
    constexpr atom& operator=(const atom&);
    constexpr atom& operator=(atom&&) noexcept;

    constexpr ~atom();

    template<typename T>
    friend constexpr auto atom_cast(const atom&) -> std::optional<T>;

    friend constexpr auto operator==(const atom&, const atom&) -> bool;

    friend constexpr void swap(atom& lhs, atom& rhs) noexcept;

    template<typename T, typename... U>
    friend constexpr auto atom_is_of_type(const atom&) -> bool;
};

class [[nodiscard]] cons {
    using sexpr_holder = std::unique_ptr<sexpr>;

    // class invariant: car_ and cdr_ always point to valid sexprs.

    sexpr_holder car_{ new sexpr{} };
    sexpr_holder cdr_{ new sexpr{} };

    [[nodiscard]]
    constexpr auto any_null_members_() const;

  public:
    template<typename T, typename U>
    constexpr cons(T&& car, U&& cdr);

    constexpr cons()  = default;
    constexpr ~cons() = default;
    constexpr cons(const cons&);
    constexpr cons(cons&&) noexcept = default;
    constexpr auto operator=(const cons&) -> cons&;
    constexpr auto operator=(cons&&) noexcept -> cons& = default;

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

constexpr auto
operator==(const atom& lhs, const atom& rhs) -> bool {
    if (lhs.no_null_members_() and rhs.no_null_members_()
        and (lhs.value_type_info_ == rhs.value_type_info_)) {
        return (*lhs.comparison_op_)(lhs.value_ptr_, rhs.value_ptr_);
    }
    return false;
}

template<not_sexpr_like T>
constexpr atom::atom(T&& value)
    : // NOLINTBEGIN{cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay}
      value_ptr_{ static_cast<void*>(new std::decay_t<T>{ std::forward<T>(value) }) },
      // NOLINTEND{cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay}
      value_deleter_{ [](void* const ptr) {
          // NOLINTBEGIN{cppcoreguidelines-owning-memory}
          delete static_cast<std::decay_t<T>* const>(ptr);
          // NOLINTEND{cppcoreguidelines-owning-memory}
      } },
      comparison_op_{ [](void const* const lhs_ptr, void const* const rhs_ptr) {
          using ptr_type  = std::decay_t<T> const* const;
          const auto& lhs = *static_cast<ptr_type>(lhs_ptr);
          const auto& rhs = *static_cast<ptr_type>(rhs_ptr);
          return lhs == rhs;
      } },
      clone_op_{ [](void const* const src_ptr) {
          const auto& src = *static_cast<std::decay_t<T> const* const>(src_ptr);
          return static_cast<void*>(new std::decay_t<T>{ src });
      } },
      value_type_info_{ &typeid(std::decay_t<T>) } {}

constexpr auto
atom::no_null_members_() const -> bool {
    return std::invoke([]<typename... T>(T... p) static { return ((nullptr != p) and ...); },
                       this->value_ptr_,
                       this->value_deleter_,
                       this->comparison_op_,
                       this->clone_op_,
                       this->value_type_info_);
}

constexpr void
atom::set_null_() {
    this->value_ptr_       = nullptr;
    this->value_deleter_   = nullptr;
    this->comparison_op_   = nullptr;
    this->clone_op_        = nullptr;
    this->value_type_info_ = nullptr;
}

constexpr atom::~atom() {
    if (this->no_null_members_()) {
        (*this->value_deleter_)(this->value_ptr_);
        this->set_null_();
    }
}

constexpr atom::atom(const atom& other)
    : value_ptr_{ (*other.clone_op_)(other.value_ptr_) },
      value_deleter_{ other.value_deleter_ },
      comparison_op_{ other.comparison_op_ },
      clone_op_{ other.clone_op_ },
      value_type_info_{ other.value_type_info_ } {}

constexpr atom::atom(atom&& other) noexcept { swap(*this, other); }

constexpr atom&
atom::operator=(const atom& other) {
    auto tmp     = other; // Makes sure self assignment is benign.
    return *this = std::move(tmp);
}

constexpr atom&
atom::operator=(atom&& other) noexcept {
    swap(*this, other);
    return *this;
}

template<typename T>
constexpr auto
atom_cast(const atom& x) -> std::optional<T> {
    if (x.no_null_members_() and typeid(T) == *x.value_type_info_) {
        return *static_cast<T*>(x.value_ptr_);
    }
    return {};
}

constexpr void
swap(atom& lhs, atom& rhs) noexcept {
    std::swap(lhs.value_ptr_, rhs.value_ptr_);
    std::swap(lhs.value_deleter_, rhs.value_deleter_);
    std::swap(lhs.comparison_op_, rhs.comparison_op_);
    std::swap(lhs.clone_op_, rhs.clone_op_);
    std::swap(lhs.value_type_info_, rhs.value_type_info_);
}

template<typename T, typename... U>
[[nodiscard]]
constexpr auto
atom_is_of_type(const atom& x) -> bool {
    if (x.no_null_members_()) {
        return *x.value_type_info_ == typeid(T) or ((*x.value_type_info_ == typeid(U)) or ...);
    }
    return false;
}

template<typename T, typename U>
constexpr cons::cons(T&& car, U&& cdr)
    : car_{ std::make_unique<sexpr>(std::forward<T>(car)) },
      cdr_{ std::make_unique<sexpr>(std::forward<U>(cdr)) } {}

constexpr cons::cons(const cons& other) {
    this->car_ = std::make_unique<sexpr>(*other.car_);
    this->cdr_ = std::make_unique<sexpr>(*other.cdr_);
}

constexpr auto
cons::operator=(const cons& other) -> cons& {
    return *this = auto{ other }; // Makes sure self assignment is benign.
}

constexpr auto
cons::car() const -> const sexpr& {
    return *this->car_;
}

constexpr auto
cons::cdr() const -> const sexpr& {
    return *this->cdr_;
}

constexpr auto
operator==(const cons& lhs, const cons& rhs) -> bool {
    const auto op =
        tyvi::sstd::overloaded{ []<sexpr_like T>(const T& lhs, const T& rhs) { return lhs == rhs; },
                                [](auto&&, auto&&) { return false; } };

    return std::visit(op, lhs.car(), rhs.car()) and std::visit(op, lhs.cdr(), rhs.cdr());
}

} // namespace tyvi::actions
