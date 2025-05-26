#include "spdlog/spdlog.h"

#include "tyvi/tyvi_hello_world.h"

namespace tyvi {
int
hello_world() {
    spdlog::info("Hello World!");
    spdlog::error("Hello World error!");
    spdlog::warn("Hello World warn!");
    spdlog::critical("Hello World critical!");
    return 1;
}
} // namespace tyvi
