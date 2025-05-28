#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "tyvi/mdspan.h"

namespace tyvi {

template<typename T>
struct mdgrid_element_descriptor {
    using element_type = T;

    std::size_t rank, dim;
};

template<auto ElemDesc, typename Extents, typename GridLayoutPolicy = std::layout_right>
class [[nodiscard]]
mdgrid {
    using grid_layout_mapping_type = GridLayoutPolicy::template mapping<Extents>;

    grid_layout_mapping_type grid_mapping_;

  public:
    template<typename... OtherIndexTypes>
    explicit constexpr mdgrid(OtherIndexTypes&&... grid_size)
        : grid_mapping_(Extents(std::forward<OtherIndexTypes>(grid_size)...)) {}
};

} // namespace tyvi
