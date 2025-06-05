#pragma once

#include <array>
#include <concepts>
#include <cstddef>
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

template<auto ElemDesc, typename Extents, typename GridLayoutPolicy = std::layout_right>
class [[nodiscard]]
mdgrid {
    using value_type = decltype(ElemDesc)::value_type;

    using ElementExtents      = sstd::geometric_extents<ElemDesc.rank, ElemDesc.dim>;
    using ElementLayoutPolicy = std::layout_right;

    using device_vec = thrust::device_vector<value_type>;
    using device_buffer =
        mdgrid_buffer<device_vec, ElementExtents, ElementLayoutPolicy, Extents, GridLayoutPolicy>;

    using staging_vec = thrust::host_vector<value_type>;
    using staging_buffer =
        mdgrid_buffer<staging_vec, ElementExtents, ElementLayoutPolicy, Extents, GridLayoutPolicy>;

    device_buffer device_buff_;
    staging_buffer staging_buff_;

    template<typename Future>
    class work_chain;

  public:
    explicit constexpr mdgrid(const auto... grid_extents)
        : device_buff_(grid_extents...),
          staging_buff_(grid_extents...) {}

    [[nodiscard]]
    auto view_staging_buff(this auto& self) {
        return self.staging_buff_.mds();
    }

    [[nodiscard]]
    auto init_work() const& {
        // Crate empty future which is ready but empty.
        // We don't care about the value so use just some type (i.e. int).
        return work_chain(thrust::device_future<int>(thrust::new_stream));
    };

    auto init_work() && = delete;
};

/// Move only DAG representing dependencies between async work.
template<auto ElemDesc, typename Extents, typename GridLayoutPolicy>
template<typename Future>
class [[nodiscard]]
mdgrid<ElemDesc, Extents, GridLayoutPolicy>::work_chain {
    friend class mdgrid<ElemDesc, Extents, GridLayoutPolicy>;

    std::remove_cvref_t<Future> future_;

    explicit work_chain(Future&& future) : future_(std::move(future)) {}

  public:
    ~work_chain() noexcept = default;

    work_chain(work_chain&&) noexcept                  = default; // move constructor
    work_chain& operator=(work_chain&& other) noexcept = default; // move assignment

    work_chain(const work_chain&)            = delete; // copy constructor
    work_chain& operator=(const work_chain&) = delete; // copy assignment

    template<typename MDG, typename F>
    auto for_each(MDG& mdg, F&& f) {
        auto grid_mds  = mdg.device_buff_.mds();
        auto wrapped_f = [grid_mds, f = std::forward<F>(f)](const auto& idx) { f(grid_mds[idx]); };
        const auto indices = sstd::index_space(grid_mds);

        return typename MDG::work_chain(thrust::async::for_each(thrust::device.after(future_),
                                                                indices.begin(),
                                                                indices.end(),
                                                                std::move(wrapped_f)));
    }

    template<typename MDG>
    auto sync_to_staging(MDG& mdg) {
        return typename MDG::work_chain(thrust::async::copy(thrust::device.after(future_),
                                                            mdg.device_buff_.begin(),
                                                            mdg.device_buff_.end(),
                                                            mdg.staging_buff_.begin()));
    }

    template<typename MDG>
    auto sync_from_staging(MDG& mdg) {
        return typename MDG::work_chain(thrust::async::copy(thrust::device.after(future_),
                                                            mdg.staging_buff_.begin(),
                                                            mdg.staging_buff_.end(),
                                                            mdg.device_buff_.begin()));
    }

    void wait() { future_.wait(); }
};

} // namespace tyvi
