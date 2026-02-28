#pragma once

namespace tyvi::backend {

struct cpu_tag {};
struct hip_tag {};

#ifdef TYVI_USE_CPU_BACKEND
using active_tag = cpu_tag;
constexpr bool is_cpu = true;
constexpr bool is_hip = false;
#elif defined(TYVI_USE_HIP_BACKEND)
using active_tag = hip_tag;
constexpr bool is_cpu = false;
constexpr bool is_hip = true;
#endif

} // namespace tyvi::backend
