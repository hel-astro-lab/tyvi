#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "tyvi/mdspan.h"

namespace tyvi {

template<typename V, typename ElemExtents, typename ElemLP, typename GridExtents, typename GridLP>
class [[nodiscard]] mdgrid_buffer {
    using grid_mapping_type = GridLP::template mapping<GridExtents>;

    grid_mapping_type grid_mapping_;

  public:
    explicit constexpr mdgrid_buffer(const grid_mapping_type& m) : grid_mapping_(m) {}
};

} // namespace tyvi
