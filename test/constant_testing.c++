/** @file
 * Implementation of facility for constant testing extending ut testing.
 *
 * Constant test is constantly evaluated.
 */

#include <format>
#include <source_location>
#include <span>
#include <string>

#include "constant_testing.h"

namespace tyvi {

auto
format_location(const std::source_location location) -> std::string {
    return std::format("{}:{}:{}", location.file_name(), location.line(), location.column());
}

/// Formats error message from failed constant tests
auto
format_errors(std::span<Tester::error_value_type const> errors) -> std::string {
    auto message = std::string{ "Failed constant tests:" };
    for (const auto error : errors) {
        if (error.has_value()) { message += std::format("\n\t{}", format_location(error.value())); }
    }
    // Add trailing newlines
    message += "\n\n";
    return message;
}

} // namespace tyvi
