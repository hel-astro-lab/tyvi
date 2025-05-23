#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include "sample_library.h"

void
test_logging(const int x) {
    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", x);

    const auto magic_int   = 43;
    const auto magic_float = 1.23456;

    spdlog::warn("Easy padding in numbers like {:08d}", magic_int);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", magic_int);
    spdlog::info("Support for floats {:03.2f}", magic_float);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:<30}", "left aligned");

    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::debug("This message should be displayed..");

    // change log pattern
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

    // Compile time log levels
    // Note that this does not change the current log level, it will only
    // remove (depending on SPDLOG_ACTIVE_LEVEL) the call on the release code.
    SPDLOG_TRACE("Some trace message with param {}", x);
    SPDLOG_DEBUG("Some debug message");
}
