#pragma once

#include <ranges>
#include <tuple>

#include "thrust/device_vector.h"
#include "thrust/equal.h"
#include "thrust/execution_policy.h"
#include "thrust/host_vector.h"
#include "thrust/random.h"
#include "thrust/random/uniform_int_distribution.h"
#include "thrust/transform.h"

namespace tyvi {

/// Sanity checks that thrust is usable.
inline bool
thrust_test() {
    const auto N = 64uz;

    using I = int;

    const auto [vec, vec2_correct] = [&] {
        thrust::default_random_engine rng;
        thrust::uniform_int_distribution<I> dist(-4, 5);

        auto hvec          = thrust::host_vector<I>(N);
        auto hvec2_correct = thrust::host_vector<I>(N);

        for (const auto i : std::views::iota(0uz, N)) {
            hvec[i]          = dist(rng);
            hvec2_correct[i] = hvec[i] * hvec[i];
        }

        return std::tuple{ thrust::device_vector<I>(hvec),
                           thrust::device_vector<I>(hvec2_correct) };
    }();

    auto vec2 = thrust::device_vector<I>(N);

    std::ignore =
        thrust::transform(thrust::device, vec.begin(), vec.end(), vec2.data(), [](const auto x) {
            return x * x;
        });

    return thrust::equal(thrust::device, vec2.begin(), vec2.end(), vec2_correct.begin());
}

} // namespace tyvi
