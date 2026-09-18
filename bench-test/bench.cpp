#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thrust/sequence.h>

template <std::size_t N>
double work() {
    using T = float;
    auto vec = thrust::device_vector<T>(10000000);
    const auto w = tyvi::mdgrid_work{};
    thrust::sequence(w.on_this(), vec.begin(), vec.end());

    // begin measure
    const auto start = std::chrono::steady_clock::now();
    thrust::for_each(w.on_this(), vec.begin(), vec.end(), [](auto& x) {
        using T = std::remove_cvref_t<decltype(x)>;
        T sum = T{1.0};
        T term = T{1.0};
        for (std::size_t n = 1; n <= N; n++) {
            term *= x / n;
            sum += term;
        }
        x = sum;
    });
    w.wait();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> diff = end - start;

    const auto host_vec = thrust::host_vector<T>(vec.begin(), vec.end());

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
