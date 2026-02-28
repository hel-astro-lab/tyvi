#pragma once

#include "tyvi/backend.h"

#ifdef TYVI_USE_CPU_BACKEND
#include "tyvi/dynamic_array_cpu.h"
#elif defined(TYVI_USE_HIP_BACKEND)
#include "thrust/device_vector.h"
#include "thrust/host_vector.h"
#endif

namespace tyvi {

template<typename T>
#ifdef TYVI_USE_CPU_BACKEND
using device_vector = dynamic_array<T>;
#elif defined(TYVI_USE_HIP_BACKEND)
using device_vector = thrust::device_vector<T>;
#endif

template<typename T>
#ifdef TYVI_USE_CPU_BACKEND
using host_vector = dynamic_array<T>;
#elif defined(TYVI_USE_HIP_BACKEND)
using host_vector = thrust::host_vector<T>;
#endif

} // namespace tyvi
