#pragma once

#include <cstddef>
#include <utility>

#include <experimental/mdspan>

namespace tyvi::sstd {

/// Geometric extents are all static and the same.
template<std::size_t rank, std::size_t dim>
using geometric_extents = decltype(std::invoke(
    []<std::size_t... I>(std::index_sequence<I...>) {
        return std::extents<std::size_t, (0 * I + dim)...>{};
    },
    std::make_index_sequence<rank>()));

template<typename T,
         std::size_t rank,
         std::size_t dim,
         typename LayoutPolicy   = std::layout_right,
         typename AccessorPolicy = std::default_accessor<T>>
using geometric_mdspan = std::mdspan<T, geometric_extents<rank, dim>, LayoutPolicy, AccessorPolicy>;
} // namespace tyvi::sstd
