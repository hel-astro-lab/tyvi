#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thrust/sequence.h>

using T = float;

auto normalize(auto x, auto n) {
    return T{ 1.0 } + (5.0 - 1.0) * static_cast<T>(x) / static_cast<T>(n);
}

void
verify(const auto& vec) {
    const T limit    = 0.01f;
    bool approx_same = true;
    for (auto i = 0uz; i < vec.size(); i++) {
        const auto ref = std::exp(normalize(i, vec.size()));
        const bool less =
            limit > std::abs(ref - vec[i]);
        approx_same &= less;
        if (not less) {
            std::printf("Not less than limit for %d: %f, (ref = %f) (limit = %f)\n",
                        i,
                        vec[i],
                        ref,
                        limit);
        }
    }
    assert(approx_same);
}

template <std::size_t N>
double work() {
    static constexpr std::size_t M = 10000000;
    auto vec = thrust::device_vector<T>(M);
    const auto w = tyvi::mdgrid_work{};
    thrust::sequence(w.on_this(), vec.begin(), vec.end());

    const auto start = std::chrono::steady_clock::now();
    thrust::for_each(w.on_this(), vec.begin(), vec.end(), [](auto& x) {
        using T = std::remove_cvref_t<decltype(x)>;
        T sum = T{1.0};
        T term = T{1.0};
        const T y = normalize(x, M);
        for (std::size_t n = 1; n <= N; n++) {
            term *= y / n;
            sum += term;
        }
        x = sum;
    });
    w.wait();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> diff = end - start;

    if constexpr (N == 16) {
        const auto host_vec = thrust::host_vector<T>(vec.begin(), vec.end());
        verify(host_vec);
    }

    return diff.count();
}

template <std::size_t N>
double measure() {
    double total_time = 0.0;
    static constexpr std::size_t n = 10;
    for (auto i = 0uz; i < n; i++) {
        const double runtime = work<N>();
        total_time += i == 0 ? 0.0 : runtime;
    }

    return total_time;
}

int main() {
    const std::array<double, 5> measurements {
        measure<1>(),
        measure<2>(),
        measure<4>(),
        measure<8>(),
        measure<16>(),
    };

    for (auto i = 0uz; i < measurements.size(); i++) {
        std::printf("%d, %f\n", 1u << i, measurements[i]);
    }

    return 0;
}
