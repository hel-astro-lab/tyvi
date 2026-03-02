#pragma once

namespace tyvi::backend {

struct cpu_tag {};
struct hip_tag {};

#ifdef TYVI_USE_CPU_BACKEND
using active_tag = cpu_tag;
#elif defined(TYVI_USE_HIP_BACKEND)
using active_tag = hip_tag;
#endif

} // namespace tyvi::backend
