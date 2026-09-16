// Copyright 2026 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace tyvi {

enum class backend : std::uint8_t { cpu, hip, cuda };

#if defined(TYVI_BACKEND_CPU)
static constexpr auto active_backend = backend::cpu;
#elif defined(TYVI_BACKEND_HIP)
static constexpr auto active_backend = backend::hip;
#elif defined(TYVI_BACKEND_CUDA)
static constexpr auto active_backend = backend::cuda;
#else
static_assert(false, "Unregonized backend!");
#endif

} // namespace tyvi
