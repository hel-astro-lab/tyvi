#pragma once

#include <cstdint>

namespace tyvi {

enum class backend : std::uint8_t { cpu, hip };

#if defined(TYVI_BACKEND_CPU)
static constexpr auto active_backend = backend::cpu;
#elif defined(TYVI_BACKEND_HIP)
static constexpr auto active_backend = backend::hip;
#else
static_assert(false, "Unregonized backend!");
#endif

} // namespace tyvi
