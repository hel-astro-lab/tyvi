#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <utility>

#include "tyvi/backend.h"

#include "tyvi/containers.h"

#ifdef TYVI_USE_CPU_BACKEND
#include <algorithm>
#elif defined(TYVI_USE_HIP_BACKEND)
#include <deque>
#include <future>
#include <mutex>
#include "thrust/copy.h"
#include "thrust/execution_policy.h"
#include "thrust/for_each.h"
#include "hip/hip_runtime.h"
#endif

#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"
#include "tyvi/sstd.h"

namespace tyvi {

template<typename T>
    requires std::same_as<T, std::remove_cvref_t<T>>
struct mdgrid_element_descriptor {
    using value_type = T;

    std::size_t rank, dim;
};

class mdgrid_work;

template<auto ElemDesc, typename GridExtents, typename GridLayoutPolicy = std::layout_right>
class [[nodiscard]]
mdgrid {
  public:
    using value_type = decltype(ElemDesc)::value_type;

    using element_extents_type = sstd::geometric_extents<ElemDesc.rank, ElemDesc.dim>;
    using element_layout_type  = std::layout_right;

    using grid_extents_type = GridExtents;
    using grid_layout_type  = GridLayoutPolicy;

    using device_vec  = device_vector<value_type>;
    using staging_vec = host_vector<value_type>;

    using device_buffer = mdgrid_buffer<device_vec,
                                        element_extents_type,
                                        element_layout_type,
                                        grid_extents_type,
                                        grid_layout_type>;

    using staging_buffer = mdgrid_buffer<staging_vec,
                                         element_extents_type,
                                         element_layout_type,
                                         grid_extents_type,
                                         grid_layout_type>;

  private:
#ifdef TYVI_USE_CPU_BACKEND
    device_buffer buff_;

    constexpr auto& primary_buffer() { return buff_; }
    constexpr const auto& primary_buffer() const { return buff_; }
    constexpr auto& staging_buffer_ref() { return buff_; }
    constexpr const auto& staging_buffer_ref() const { return buff_; }
#elif defined(TYVI_USE_HIP_BACKEND)
    device_buffer device_buff_;
    staging_buffer staging_buff_;

    constexpr auto& primary_buffer() { return device_buff_; }
    constexpr const auto& primary_buffer() const { return device_buff_; }
    constexpr auto& staging_buffer_ref() { return staging_buff_; }
    constexpr const auto& staging_buffer_ref() const { return staging_buff_; }
#endif

    friend class mdgrid_work;

  public:
#ifdef TYVI_USE_CPU_BACKEND
    explicit constexpr mdgrid(const auto... grid_extents)
        : buff_(grid_extents...) {}

    explicit constexpr mdgrid(const grid_extents_type& grid_extents)
        : buff_(grid_extents) {}
#elif defined(TYVI_USE_HIP_BACKEND)
    explicit constexpr mdgrid(const auto... grid_extents)
        : device_buff_(grid_extents...),
          staging_buff_(grid_extents...) {}

    explicit constexpr mdgrid(const grid_extents_type& grid_extents)
        : device_buff_(grid_extents),
          staging_buff_(grid_extents) {}
#endif

    [[nodiscard]]
    constexpr auto mds() & {
        return primary_buffer().mds();
    }

    [[nodiscard]]
    constexpr auto mds() const& {
        return primary_buffer().mds();
    }

    [[nodiscard]]
    constexpr auto staging_mds() & {
        return staging_buffer_ref().mds();
    }

    [[nodiscard]]
    constexpr auto staging_mds() const& {
        return staging_buffer_ref().mds();
    }

    [[nodiscard]]
    constexpr grid_extents_type extents() const {
        return primary_buffer().grid_extents();
    }

    /// Get span to the underlying data buffer.
    [[nodiscard]]
    constexpr auto span() & {
        return primary_buffer().span();
    }

    /// Get span to the underlying data buffer.
    [[nodiscard]]
    constexpr auto span() const& {
        return primary_buffer().span();
    }

    /// Get span to the underlying data buffer in staging buffer.
    [[nodiscard]]
    constexpr auto staging_span() & {
        return staging_buffer_ref().span();
    }

    /// Get span to the underlying data buffer in staging buffer.
    [[nodiscard]]
    constexpr auto staging_span() const& {
        return staging_buffer_ref().span();
    }

    /// Get copy of the underlying data buffer from staging buffer.
    [[nodiscard]]
    constexpr staging_vec underlying_staging_buffer() const {
        return staging_buffer_ref().underlying_buffer();
    }

    /// Get copy of the underlying data buffer.
    [[nodiscard]]
    constexpr device_vec underlying_buffer() const {
        return primary_buffer().underlying_buffer();
    }

    /// Set the underlying data buffer in staging buffer.
    ///
    /// Invalidates all pointers. This includes pointers in [md]span.
    ///
    /// Constraints are checked in mdgrid_buffer method.
    ///
    /// Throws if the given buffer is not the same legth as
    /// the one it replaces.
    constexpr void set_underlying_staging_buffer(auto&& buff) {
        staging_buffer_ref().set_underlying_buffer(std::forward<decltype(buff)>(buff));
    }

    /// Set the underlying data buffer.
    ///
    /// Invalidates all pointers to the staging buffer.
    /// This includes pointers in [md]span.
    ///
    /// Constraints are checked in mdgrid_buffer method.
    ///
    /// Throws if the given buffer is not the same legth as
    /// the one it replaces.
    constexpr void set_underlying_buffer(auto&& buff) {
        primary_buffer().set_underlying_buffer(std::forward<decltype(buff)>(buff));
    }

    constexpr void invalidating_resize(const grid_extents_type& extents) {
        primary_buffer().invalidating_resize(extents);
        if constexpr (!backend::is_cpu) {
            staging_buffer_ref().invalidating_resize(extents);
        }
    }

    template<typename... Indices>
        requires std::constructible_from<grid_extents_type, Indices...>
    constexpr void invalidating_resize(Indices... indices) {
        this->invalidating_resize(grid_extents_type{ std::forward<Indices>(indices)... });
    }
};

// ============================================================================
// CPU backend: sequential mdgrid_work with SIMD-friendly nested loops
// ============================================================================
#ifdef TYVI_USE_CPU_BACKEND

namespace detail {

/// Compile-time recursive nested-loop generator.
///
/// Produces:  for (dim0) for (dim1) ... #pragma omp simd for (dimR-1) f(idx)
///
/// The innermost dimension is annotated with #pragma omp simd so the
/// compiler vectorizes stride-1 accesses through the mdspan accessor chain.
template<std::size_t Dim, std::size_t Rank, typename Extents, typename F>
void nested_for(const Extents& ext, F& f, std::array<std::size_t, Rank>& idx) {
    if constexpr (Dim == Rank - 1) {
        // Innermost loop — enable SIMD vectorization.
        const auto n = ext.extent(Dim);
        #pragma omp simd
        for (std::size_t k = 0; k < n; ++k) {
            idx[Dim] = k;
            f(idx);
        }
    } else {
        for (std::size_t i = 0; i < ext.extent(Dim); ++i) {
            idx[Dim] = i;
            nested_for<Dim + 1, Rank>(ext, f, idx);
        }
    }
}

} // namespace detail

/// No-op execution policy tag for CPU backend.
struct cpu_exec_policy {};

/// Move-only work unit — sequential execution on CPU.
class mdgrid_work {
    template<auto, typename, typename>
    friend class mdgrid;

    template<std::same_as<mdgrid_work>... T>
        requires(sizeof...(T) != 0)
    friend void when_all(const T&... w);

  public:
    [[nodiscard]]
    mdgrid_work() = default;

    ~mdgrid_work() noexcept = default;

    mdgrid_work(mdgrid_work&&) noexcept                  = default; // move constructor
    mdgrid_work& operator=(mdgrid_work&& other) noexcept = default; // move assignment

    mdgrid_work(const mdgrid_work&)            = delete; // copy constructor
    mdgrid_work& operator=(const mdgrid_work&) = delete; // copy assignment

    template<typename MDG, typename F>
    const mdgrid_work& for_each(MDG& mdg, F&& f) const {
        auto grid_mds  = mdg.primary_buffer().mds();
        auto wrapped_f = [grid_mds, f = std::forward<F>(f)](const auto& idx) { f(grid_mds[idx]); };

        constexpr auto Rank = std::remove_cvref_t<decltype(grid_mds)>::rank();
        if constexpr (Rank == 0) {
            wrapped_f(std::array<std::size_t, 0>{});
        } else {
            auto idx = std::array<std::size_t, Rank>{};
            detail::nested_for<0, Rank>(grid_mds.extents(), wrapped_f, idx);
        }

        return *this;
    }

    template<typename T, typename E, typename LP, typename AP, typename F>
    const mdgrid_work& for_each_index(const std::mdspan<T, E, LP, AP>& mds, F&& f) const {
        using MDS = std::mdspan<T, E, LP, AP>;
        constexpr auto Rank = E::rank();

        // Determine which callable signature the user provided.
        using idx_type = std::array<std::size_t, Rank>;

        // Build type-detection helpers via index_space.
        const auto indices = sstd::index_space(mds);

        using grid_indices_range = decltype(indices);
        using element_indices_range =
            decltype(sstd::index_space(std::declval<typename MDS::value_type>()));

        using grid_indices_range_reference = std::ranges::range_reference_t<grid_indices_range>;
        using element_indices_range_reference =
            std::ranges::range_reference_t<element_indices_range>;

        if constexpr (std::invocable<F, grid_indices_range_reference>) {
            // Grid-index-only callback:  f(idx)
            if constexpr (Rank == 0) {
                f(idx_type{});
            } else {
                auto ff  = std::forward<F>(f);
                auto idx = idx_type{};
                detail::nested_for<0, Rank>(mds.extents(), ff, idx);
            }
        } else if constexpr (std::invocable<F,
                                            grid_indices_range_reference,
                                            element_indices_range_reference>) {
            // Grid + element callback:  f(idx, jdx)
            auto wrapped_f = [mds, f = std::forward<F>(f)](const auto& idx) {
                const auto elem_indices = sstd::index_space(mds[idx]);
                for (const auto jdx : elem_indices) { f(idx, jdx); }
            };

            if constexpr (Rank == 0) {
                wrapped_f(idx_type{});
            } else {
                auto idx = idx_type{};
                detail::nested_for<0, Rank>(mds.extents(), wrapped_f, idx);
            }
        }

        return *this;
    }

    template<typename MDG, typename F>
    const mdgrid_work& for_each_index(MDG& mdg, F&& f) const {
        return for_each_index(mdg.primary_buffer().mds(), std::forward<F>(f));
    }

    template<typename MDG>
    const mdgrid_work& sync_to_staging(MDG& /*mdg*/) const {
        return *this;
    }

    template<typename MDG>
    const mdgrid_work& sync_from_staging(MDG& /*mdg*/) const {
        return *this;
    }

    void wait() const {}

    [[nodiscard]]
    constexpr cpu_exec_policy on_this() const { return {}; }

    template<std::size_t N>
        requires(N != 0)
    [[nodiscard]]
    std::array<mdgrid_work, N> split() const {
        return std::array<mdgrid_work, N>{};
    };
};

/// No-op synchronization on CPU.
template<std::same_as<mdgrid_work>... T>
    requires(sizeof...(T) != 0)
void
when_all(const T&... /*w*/) {}

// ============================================================================
// HIP backend: stream-based async mdgrid_work
// ============================================================================
#elif defined(TYVI_USE_HIP_BACKEND)

namespace detail {

/// Manages streams in order to promote stream reuse.
///
/// When user asks for new stream, give it via stream_hadle which destructor
/// will notify the factory that it has been freed via std::future/promise mechanism.
class stream_factory : sstd::immovable {
  public:
    using stream_t = hipStream_t;

    using stream_future  = std::future<stream_t>;
    using stream_promise = std::promise<stream_t>;

    class stream_handle {
        bool active_{ false };
        stream_t stream_;
        stream_promise promise_;

        void release_stream();

        friend class stream_factory;

        [[nodiscard]]
        explicit stream_handle(stream_t, stream_promise);

      public:
        stream_handle(stream_handle&&) noexcept;                  // move constructor
        stream_handle& operator=(stream_handle&& other) noexcept; // move assignment

        stream_handle(const stream_handle&)            = delete; // copy constructor
        stream_handle& operator=(const stream_handle&) = delete; // copy assignment

        ~stream_handle();

        [[nodiscard]]
        stream_t get() const;

        [[nodiscard]]
        thrust::hip_rocprim::execute_on_stream_nosync on_stream() const;

        void wait() const;
    };

  private:
    std::mutex mutex_;
    std::deque<stream_future> managed_streams_;

  public:
    [[nodiscard]]
    stream_handle get();
};

[[nodiscard]]
inline stream_factory&
global_stream_factory();

constexpr void
hip_check_error(const hipError_t e) {
    if (e != hipSuccess) { throw std::runtime_error{ "hipError_t != hipSuccess" }; }
}

} // namespace detail

/// Move-only DAG representing dependencies between async work.
class mdgrid_work {
    using stream_handle = detail::stream_factory::stream_handle;
    stream_handle handle_;

    template<auto, typename, typename>
    friend class mdgrid;

    template<std::same_as<mdgrid_work>... T>
        requires(sizeof...(T) != 0)
    friend void when_all(const T&... w);

  public:
    [[nodiscard]]
    explicit mdgrid_work();

    ~mdgrid_work() noexcept = default;

    mdgrid_work(mdgrid_work&&) noexcept                  = default; // move constructor
    mdgrid_work& operator=(mdgrid_work&& other) noexcept = default; // move assignment

    mdgrid_work(const mdgrid_work&)            = delete; // copy constructor
    mdgrid_work& operator=(const mdgrid_work&) = delete; // copy assignment

    template<typename MDG, typename F>
    const mdgrid_work& for_each(MDG& mdg, F&& f) const {
        auto grid_mds  = mdg.primary_buffer().mds();
        auto wrapped_f = [grid_mds, f = std::forward<F>(f)](const auto& idx) { f(grid_mds[idx]); };
        const auto indices = sstd::index_space(grid_mds);

        thrust::for_each(handle_.on_stream(), indices.begin(), indices.end(), std::move(wrapped_f));

        return *this;
    }

    template<typename T, typename E, typename LP, typename AP, typename F>
    const mdgrid_work& for_each_index(const std::mdspan<T, E, LP, AP>& mds, F&& f) const {
        using MDS = std::mdspan<T, E, LP, AP>;

        const auto indices = sstd::index_space(mds);

        using grid_indices_range = decltype(indices);
        using element_indices_range =
            decltype(sstd::index_space(std::declval<typename MDS::value_type>()));

        using grid_indices_range_reference = std::ranges::range_reference_t<grid_indices_range>;
        using element_indices_range_reference =
            std::ranges::range_reference_t<element_indices_range>;

        if constexpr (std::invocable<F, grid_indices_range_reference>) {
            thrust::for_each(handle_.on_stream(),
                             indices.begin(),
                             indices.end(),
                             std::forward<F>(f));
        } else if constexpr (std::invocable<F,
                                            grid_indices_range_reference,
                                            element_indices_range_reference>) {
            auto wrapped_f = [mds, f = std::forward<F>(f)](const auto& idx) {
                const auto elem_indices = sstd::index_space(mds[idx]);
                for (const auto jdx : elem_indices) { f(idx, jdx); }
            };

            thrust::for_each(handle_.on_stream(),
                             indices.begin(),
                             indices.end(),
                             std::move(wrapped_f));
        }

        return *this;
    }

    template<typename MDG, typename F>
    const mdgrid_work& for_each_index(MDG& mdg, F&& f) const {
        return for_each_index(mdg.primary_buffer().mds(), std::forward<F>(f));
    }

    template<typename MDG>
    const mdgrid_work& sync_to_staging(MDG& mdg) const {
        thrust::copy(handle_.on_stream(),
                     mdg.primary_buffer().begin(),
                     mdg.primary_buffer().end(),
                     mdg.staging_buffer_ref().begin());

        return *this;
    }

    template<typename MDG>
    const mdgrid_work& sync_from_staging(MDG& mdg) const {
        thrust::copy(handle_.on_stream(),
                     mdg.staging_buffer_ref().begin(),
                     mdg.staging_buffer_ref().end(),
                     mdg.primary_buffer().begin());
        return *this;
    }

    void wait() const { handle_.wait(); }

    template<std::size_t N>
        requires(N != 0)
    [[nodiscard]]
    std::array<mdgrid_work, N> split() const {
        std::array<mdgrid_work, N> new_works;

        [&]<std::size_t... I>(std::index_sequence<I...>) {
            when_all(new_works[I]...);
        }(std::make_index_sequence<N>());

        return new_works;
    };

    [[nodiscard]]
    auto on_this() const {
        return handle_.on_stream();
    }
};

/// Insersts a synchronization point between the given mdgrid_works
template<std::same_as<mdgrid_work>... T>
    requires(sizeof...(T) != 0)
void
when_all(const T&... w) {
    // https://rocm.docs.amd.com/projects/HIP/en/develop/reference/hip_runtime_api/modules/event_management.html#_CPPv415hipEventDestroy10hipEvent_t
    //
    // "Releases memory associated with the event. If the event is recording but has not completed
    //  recording when hipEventDestroy() is called, the function will return immediately
    //  and the completion_future resources will be released later, when the hipDevice is synchronized."
    //
    // This means that we can create events, use them and destroy them in this function.
    // We don't have to store them.

    std::array<hipEvent_t, sizeof...(T)> events{};

    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (detail::hip_check_error(hipEventCreate(&events[I])), ...);

        (detail::hip_check_error(hipEventRecord(events[I], w.handle_.get())), ...);

        const auto wait_all_events = [&](hipStream_t s) {
            (detail::hip_check_error(hipStreamWaitEvent(s, events[I], 0)), ...);
        };

        (wait_all_events(w.handle_.get()), ...);

        (detail::hip_check_error(hipEventDestroy(events[I])), ...);
    }(std::make_index_sequence<sizeof...(T)>());
}

#endif // TYVI_USE_CPU_BACKEND / TYVI_USE_HIP_BACKEND

} // namespace tyvi

// NOLINTBEGIN
#define TYVI_CAPTURE_SINGLE_MDS(arg) arg##_mds = arg.mds()

#define TYVI_FOR_EACH_1(action, x)       action(x)
#define TYVI_FOR_EACH_2(action, x, ...)  action(x), TYVI_FOR_EACH_1(action, __VA_ARGS__)
#define TYVI_FOR_EACH_3(action, x, ...)  action(x), TYVI_FOR_EACH_2(action, __VA_ARGS__)
#define TYVI_FOR_EACH_4(action, x, ...)  action(x), TYVI_FOR_EACH_3(action, __VA_ARGS__)
#define TYVI_FOR_EACH_5(action, x, ...)  action(x), TYVI_FOR_EACH_4(action, __VA_ARGS__)
#define TYVI_FOR_EACH_6(action, x, ...)  action(x), TYVI_FOR_EACH_5(action, __VA_ARGS__)
#define TYVI_FOR_EACH_7(action, x, ...)  action(x), TYVI_FOR_EACH_6(action, __VA_ARGS__)
#define TYVI_FOR_EACH_8(action, x, ...)  action(x), TYVI_FOR_EACH_7(action, __VA_ARGS__)
#define TYVI_FOR_EACH_9(action, x, ...)  action(x), TYVI_FOR_EACH_8(action, __VA_ARGS__)
#define TYVI_FOR_EACH_10(action, x, ...) action(x), TYVI_FOR_EACH_9(action, __VA_ARGS__)

// Macro to count number of args (up to 10)
#define TYVI_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define FOR_EACH(action, ...)        \
    TYVI_GET_MACRO(__VA_ARGS__,      \
                   TYVI_FOR_EACH_10, \
                   TYVI_FOR_EACH_9,  \
                   TYVI_FOR_EACH_8,  \
                   TYVI_FOR_EACH_7,  \
                   TYVI_FOR_EACH_6,  \
                   TYVI_FOR_EACH_5,  \
                   TYVI_FOR_EACH_4,  \
                   TYVI_FOR_EACH_3,  \
                   TYVI_FOR_EACH_2,  \
                   TYVI_FOR_EACH_1)(action, __VA_ARGS__)

/// Syntax sugar for capturing mdspans.
///
/// [foo_mds = foo.mds(), bar_mds = bar.mds()]
///
/// to
///
/// [TYVI_CMDS(foo, bar)] // CMDS = Capture MDSpan
#define TYVI_CMDS(...) FOR_EACH(TYVI_CAPTURE_SINGLE_MDS, __VA_ARGS__)
// NOLINTEND
