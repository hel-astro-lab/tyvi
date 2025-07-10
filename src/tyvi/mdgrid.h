#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <utility>

#include "thrust/async/copy.h"
#include "thrust/async/for_each.h"
#include "thrust/device_vector.h"
#include "thrust/future.h"
#include "thrust/host_vector.h"

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

template<typename Future>
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

    using device_vec    = thrust::device_vector<value_type>;
    using device_buffer = mdgrid_buffer<device_vec,
                                        element_extents_type,
                                        element_layout_type,
                                        grid_extents_type,
                                        grid_layout_type>;

    using staging_vec    = thrust::host_vector<value_type>;
    using staging_buffer = mdgrid_buffer<staging_vec,
                                         element_extents_type,
                                         element_layout_type,
                                         grid_extents_type,
                                         grid_layout_type>;

  private:
    device_buffer device_buff_;
    staging_buffer staging_buff_;

    template<typename Future>
    friend class mdgrid_work;

  public:
    explicit constexpr mdgrid(const auto... grid_extents)
        : device_buff_(grid_extents...),
          staging_buff_(grid_extents...) {}

    explicit constexpr mdgrid(const grid_extents_type& grid_extents)
        : device_buff_(grid_extents),
          staging_buff_(grid_extents) {}

    [[nodiscard]]
    constexpr auto mds(this auto& self) {
        return self.device_buff_.mds();
    }

    [[nodiscard]]
    constexpr auto staging_mds(this auto& self) {
        return self.staging_buff_.mds();
    }

    [[nodiscard]]
    constexpr grid_extents_type extents() const {
        return device_buff_.grid_extents();
    }

    /// Get span to the underlying data buffer.
    [[nodiscard]]
    constexpr auto span() const {
        return device_buff_.span();
    }

    /// Get span to the underlying data buffer in staging buffer.
    [[nodiscard]]
    constexpr auto staging_span() const {
        return staging_buff_.span();
    }

    /// Get copy of the underlying data buffer from staging buffer.
    [[nodiscard]]
    constexpr staging_vec underlying_staging_buffer() const {
        return staging_buff_.underlying_buffer();
    }

    /// Get copy of the underlying data buffer.
    [[nodiscard]]
    constexpr device_vec underlying_buffer() const {
        return device_buff_.underlying_buffer();
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
        staging_buff_.set_underlying_buffer(std::forward<decltype(buff)>(buff));
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
        device_buff_.set_underlying_buffer(std::forward<decltype(buff)>(buff));
    }
};

/// Move only DAG representing dependencies between async work.
///
/// Known limitation is that this can not represent one to many dependency.
template<typename Future = decltype(thrust::when_all())>
class [[nodiscard]] mdgrid_work {
    std::remove_cvref_t<Future> future_;

    explicit mdgrid_work(Future&& future) : future_(std::move(future)) {}

    template<auto, typename, typename>
    friend class mdgrid;

    template<typename>
    friend class mdgrid_work;

    template<typename... T>
    friend auto when_all(mdgrid_work<T>&...);

  public:
    explicit mdgrid_work() : future_(thrust::when_all()) {}

    ~mdgrid_work() noexcept = default;

    mdgrid_work(mdgrid_work&&) noexcept                  = default; // move constructor
    mdgrid_work& operator=(mdgrid_work&& other) noexcept = default; // move assignment

    mdgrid_work(const mdgrid_work&)            = delete; // copy constructor
    mdgrid_work& operator=(const mdgrid_work&) = delete; // copy assignment

    template<typename MDG, typename F>
    auto for_each(MDG& mdg, F&& f) {
        auto grid_mds  = mdg.device_buff_.mds();
        auto wrapped_f = [grid_mds, f = std::forward<F>(f)](const auto& idx) { f(grid_mds[idx]); };
        const auto indices = sstd::index_space(grid_mds);

        auto new_future = thrust::async::for_each(thrust::device.after(future_),
                                                  indices.begin(),
                                                  indices.end(),
                                                  std::move(wrapped_f));

        return mdgrid_work<decltype(new_future)>(std::move(new_future));
    }

    template<typename T, typename E, typename LP, typename AP, typename F>
    auto for_each_index(const std::mdspan<T, E, LP, AP>& mds, F&& f) {
        using MDS = std::mdspan<T, E, LP, AP>;

        const auto indices = sstd::index_space(mds);

        using grid_indices_range = decltype(indices);
        using element_indices_range =
            decltype(sstd::index_space(std::declval<typename MDS::value_type>()));

        using grid_indices_range_reference = std::ranges::range_reference_t<grid_indices_range>;
        using element_indices_range_reference =
            std::ranges::range_reference_t<element_indices_range>;

        if constexpr (std::invocable<F, grid_indices_range_reference>) {
            auto new_future = thrust::async::for_each(thrust::device.after(future_),
                                                      indices.begin(),
                                                      indices.end(),
                                                      std::forward<F>(f));

            return mdgrid_work<decltype(new_future)>(std::move(new_future));
        } else if constexpr (std::invocable<F,
                                            grid_indices_range_reference,
                                            element_indices_range_reference>) {
            auto wrapped_f = [mds, f = std::forward<F>(f)](const auto& idx) {
                const auto elem_indices = sstd::index_space(mds[idx]);
                for (const auto jdx : elem_indices) { f(idx, jdx); }
            };

            auto new_future = thrust::async::for_each(thrust::device.after(future_),
                                                      indices.begin(),
                                                      indices.end(),
                                                      std::move(wrapped_f));

            return mdgrid_work<decltype(new_future)>(std::move(new_future));
        }
    }

    template<typename MDG, typename F>
    auto for_each_index(MDG& mdg, F&& f) {
        return for_each_index(mdg.device_buff_.mds(), std::forward<F>(f));
    }

    template<typename MDG>
    auto sync_to_staging(MDG& mdg) {
        auto new_future = thrust::async::copy(thrust::device.after(future_),
                                              mdg.device_buff_.begin(),
                                              mdg.device_buff_.end(),
                                              mdg.staging_buff_.begin());

        return mdgrid_work<decltype(new_future)>(std::move(new_future));
    }

    template<typename MDG>
    auto sync_from_staging(MDG& mdg) {
        auto new_future = thrust::async::copy(thrust::device.after(future_),
                                              mdg.staging_buff_.begin(),
                                              mdg.staging_buff_.end(),
                                              mdg.device_buff_.begin());
        return mdgrid_work<decltype(new_future)>(std::move(new_future));
    }

    void wait() { future_.wait(); }
};

/// Returns a work_chain that completes when all given work_chains complete.
template<typename... T>
auto
when_all(mdgrid_work<T>&... links) {
    auto new_future = thrust::when_all(links.future_...);
    return mdgrid_work<decltype(new_future)>(std::move(new_future));
}

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
