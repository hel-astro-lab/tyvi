#pragma once

#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <ranges>
#include <utility>
#include <vector>

#include "tyvi/execution.h"

#include "mdspan.h"

namespace tyvi {

template<std::movable T, tyvi::sstd::mds_extents Extents>
class [[nodiscard]]
distributed_grid {
    using index_type      = typename Extents::index_type;
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;

    static constexpr std::size_t rank = Extents::rank();

  private:
    template<typename... I>
    [[nodiscard]]
    constexpr std::size_t mapping(I&&...) const;

    Extents extents_;

    std::vector<T> data_{};

  public:
    template<typename... I>
    constexpr explicit distributed_grid(I&&...);

    [[nodiscard]]
    constexpr Extents extents() const;

    [[nodiscard]]
    constexpr std::ranges::random_access_range auto local_indices() const;

    template<typename U, typename I, std::size_t N>
        requires(N == Extents::rank())
    constexpr auto get(const std::array<I, N>&);

    template<typename U, typename I, std::size_t N>
        requires(N == Extents::rank())
    constexpr auto get(const std::array<I, N>&) const;

    /// Returns sender of reference to the element at given inidices.
    template<typename U, typename... I>
    constexpr auto get(I&&...);

    /// Returns sender of const reference to the element at given inidices.
    template<typename U, typename... I>
    constexpr auto get(I&&...) const;
};

template<std::movable T, tyvi::sstd::mds_extents Extents, typename... I>
constexpr /* tyvi::exec::sender */ auto
make_distributed_grid(I&&...);

// implementation:

template<std::movable T, tyvi::sstd::mds_extents Extents, typename... I>
constexpr /* tyvi::exec::sender */ auto
make_distributed_grid(I&&... i) {
    return tyvi::exec::just(std::forward<I>(i)...) | tyvi::exec::then([]<typename... J>(J&&... j) {
               return distributed_grid<T, Extents>(std::forward<J>(j)...);
           });
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename... I>
constexpr distributed_grid<T, Extents>::distributed_grid(I&&... extents)
    : extents_{ std::forward<I>(extents)... } {
    data_.resize(std::layout_left::template mapping<Extents>{ extents_ }.required_span_size());
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
constexpr Extents
distributed_grid<T, Extents>::extents() const {
    return this->extents_;
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
constexpr std::ranges::random_access_range auto
distributed_grid<T, Extents>::local_indices() const {
    return tyvi::sstd::index_space(std::layout_left::template mapping<Extents>{ extents_ });
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename... I>
constexpr std::size_t
distributed_grid<T, Extents>::mapping(I&&... idx) const {
    return std::layout_left::template mapping<Extents>{ extents() }(std::forward<I>(idx)...);
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename U, typename I, std::size_t N>
    requires(N == Extents::rank())
constexpr auto
distributed_grid<T, Extents>::get(const std::array<I, N>& idx) {
    return [&]<std::size_t... J>(std::index_sequence<J...>) {
        return tyvi::exec::just(std::ref(data_[mapping(idx[J]...)]));
    }(std::make_index_sequence<N>());
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename U, typename I, std::size_t N>
    requires(N == Extents::rank())
constexpr auto
distributed_grid<T, Extents>::get(const std::array<I, N>& idx) const {
    return [&]<std::size_t... J>(std::index_sequence<J...>) {
        return tyvi::exec::just(std::cref(data_[mapping(idx[J]...)]));
    }(std::make_index_sequence<N>());
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename U, typename... I>
constexpr auto
distributed_grid<T, Extents>::get(I&&... idx) {
    return tyvi::exec::just(std::ref(data_[mapping(std::forward<I>(idx)...)]));
}

template<std::movable T, tyvi::sstd::mds_extents Extents>
template<typename U, typename... I>
constexpr auto
distributed_grid<T, Extents>::get(I&&... idx) const {
    return tyvi::exec::just(std::cref(data_[mapping(std::forward<I>(idx)...)]));
}

} // namespace tyvi
