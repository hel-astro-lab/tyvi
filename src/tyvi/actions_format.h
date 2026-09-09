// Copyright 2026 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <format>

#include "tyvi/actions_ast.h"

namespace tyvi::sstd {
template<typename CharT>
[[nodiscard]]
constexpr auto
assert_no_format_spec(std::basic_format_parse_context<CharT>& parse_ctx) {
    /* From cppreference:
           > If format-spec is not present or empty, then either
           > parse_ctx.begin() == parse_ctx.end() or *parse_ctx.begin() == '}'. */
    const auto it    = parse_ctx.begin();
    const auto empty = parse_ctx.begin() == parse_ctx.end() or *parse_ctx.begin() == '}';
    if (not empty) { throw std::format_error("invalid format: only supports empty `format-spec`"); }
    return it;
}
} // namespace tyvi::sstd

// NOLINTBEGIN{bugprone-std-namespace-modification,cert-dcl58-cpp}

template<typename CharT>
struct std::formatter<tyvi::actions::null_type, CharT> {
    static constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return tyvi::sstd::assert_no_format_spec(ctx);
    }

    template<typename FormatContext>
    static constexpr auto format(const tyvi::actions::null_type&, FormatContext& ctx) {
        return std::format_to(ctx.out(), "()");
    }
};

template<>
struct std::formatter<tyvi::actions::atom, char> {
    static constexpr auto parse(std::basic_format_parse_context<char>& ctx) {
        return tyvi::sstd::assert_no_format_spec(ctx);
    }

    template<typename FormatContext>
    static constexpr auto format(const tyvi::actions::atom& atom, FormatContext& ctx) {
        return std::format_to(ctx.out(), "{}", atom.format());
    }
};

template<typename CharT>
struct std::formatter<tyvi::actions::cons, CharT> {
    static constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return tyvi::sstd::assert_no_format_spec(ctx);
    }

    template<typename FormatContext>
    static constexpr auto format(const tyvi::actions::cons& cons, FormatContext& ctx) {
        return std::format_to(ctx.out(), "({} . {})", cons.car(), cons.cdr());
    }
};

template<typename CharT>
struct std::formatter<tyvi::actions::sexpr, CharT> {
    static constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return tyvi::sstd::assert_no_format_spec(ctx);
    }

    template<typename FormatContext>
    static constexpr auto format(const tyvi::actions::sexpr& sexpr, FormatContext& ctx) {
        auto f = [](const auto& x) { return std::format("{}", x); };
        return std::format_to(ctx.out(), "{}", std::visit(f, sexpr));
    }
};

// NOLINTEND{bugprone-std-namespace-modification,cert-dcl58-cpp}
