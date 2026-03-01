#pragma once

/** @file
 * Header for implementation of facility for constant testing extending ut testing.
 *
 * Constant test is defined to be test which is constantly evaluated.
 */

#include <array>
#include <concepts>
#include <cstddef>
#include <exception>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>

#include <boost/ut.hpp> // import boost.ut;

namespace tyvi {

/// Used to collect test results from constant tests (ut)
/**
 * Usage is similar to sycl::handler:
 *
 * \code{.cpp}
 * queue.submit([&](sycl::handler& hanler) { // commad group scope };
 * \endcode
 *
 * but:
 *    queue.sumbit -> idg::constant_testing::constant_testing
 *    sycl::handler& -> idg::constant_testing::Tester& (optionally auto&)
 *    command group scope -> constant test
 *    if (constant test) { ut::expect -> tester.expect }
 *
 *  see documentation of constant testing
 */
class Tester {
  public:
    /// Maximum number of failed tests in one constant_testing enviroment
    static constexpr std::size_t max_number_of_failed_tests = 10;
    static constexpr std::source_location max_number_of_failed_tests_marker =
        std::source_location::current();
    using error_value_type = std::optional<std::source_location>;
    using error_array      = std::array<error_value_type, max_number_of_failed_tests>;

  private:
    bool success_              = true;
    error_array errors_        = {};
    error_array::iterator pos_ = errors_.begin();

  public:
    [[nodiscard]]
    constexpr bool is_success() const {
        return success_;
    }

    static constexpr void throw_max_number_of_test_error() {
        // Can not yet format constexpr error message,
        // so can not use the marker.
        // see: https://github.com/cplusplus/papers/issues/1445

        // using namespace std::literals;
        // using str      = std::string;
        // const auto& sl = Tester::max_number_of_failed_tests_marker;

        // const auto info =
        //     "More errors than fit in Tester::errors_ (see: Tester::max_number_of_failed_tests_marker "s
        //     + str{ sl.file_name() } + ":"s + str{ sl.line() } + ")"s;

        throw std::logic_error("amount of tests > Tester::max_number_of_failed_tests)");
    }

    constexpr void expect(const bool result,
                          std::source_location location = std::source_location::current()) {
        success_ = success_ and result;

        if (not result) {
            if (pos_ == errors_.end()) { throw_max_number_of_test_error(); }
            *pos_ = location;
            pos_  = std::next(pos_);
        }
    }

    [[nodiscard]]
    constexpr const auto& get_errors() const {
        return errors_;
    }
};

/// Return type of function \ref do_testing
struct do_testing_result_type {
    bool value{};
    Tester::error_array errors{};
};

template<std::invocable<Tester&> F>
consteval auto
do_testing() -> do_testing_result_type {
    auto tester = Tester{};
    F::operator()(tester);
    return { .value = tester.is_success(), .errors = tester.get_errors() };
}

/// std::source_location -> "<file_name>:<line>:<column>"
auto
format_location(std::source_location location) -> std::string;

/// Formats error message from failed constant tests
auto
format_errors(std::span<Tester::error_value_type const> errors) -> std::string;

/// `[](Tester&) static {...}`
template<typename F>
concept constant_testable =
    std::invocable<F, Tester&> and requires(F f, Tester t) { F::operator()(t); };

/// Enviroment for constant testing
/**
 * Has to be called inside ut test.
 *
 * Example usage:
 *
 *     #include <boost/ut.hpp>
 *     int main() {
 *         using namespace boost:ut;
 *         using namespace idg::constant_testing;
 *         "constant test"_test = [] {
 *             constant_testing([](auto& tester) static consteval {
 *                 // This lambda is constant evaluated
 *                 tester.expect(<constantly_evaluated_predicate>);
 *             });
 *         };
 *     }
 */
template<constant_testable F>
constexpr void
constant_testing(F) {
    static constexpr auto results = std::invoke([] { return do_testing<F>(); });
    boost::ut::expect(results.value) << format_errors(results.errors);
}

} // namespace tyvi
