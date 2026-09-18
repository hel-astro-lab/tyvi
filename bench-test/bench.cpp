#include "tyvi/mdgrid.h"
#include "tyvi/mdspan.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thrust/sequence.h>

double measure() {
    using T = float;
    auto vec = thrust::device_vector<T>(1000000);
    const auto w = tyvi::mdgrid_work{};
    thrust::sequence(w.on_this(), vec.begin(), vec.end());

    // begin measure
    const auto start = std::chrono::steady_clock::now();
    static constexpr std::size_t N = 1uz;
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

int main() {

    double total_time = 0.0;
    static constexpr std::size_t n = 10;
    for (auto i = 0uz; i < n; i++) {
        const double runtime = measure();
        total_time += i == 0 ? 0.0 : runtime;
    }

    std::printf("Time taken: %f\n", total_time / static_cast<double>(n));
    return 0;
}
