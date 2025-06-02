#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "tyvi/mdspan.h"

namespace tyvi {

template<typename V, typename ElemExtents, typename ElemLP, typename GridExtents, typename GridLP>
class [[nodiscard]] mdgrid_buffer {
    /// Element type of grid is always mdspan, so element element type is the innermost element type.
    using element_element_type = V::value_type;
    using element_mapping_type = ElemLP::template mapping<ElemExtents>;
    using grid_mapping_type    = GridLP::template mapping<GridExtents>;

    static_assert(ElemExtents::rank_dynamic() == 0);
    static_assert(element_mapping_type::is_always_exhaustive());

    static constexpr element_mapping_type element_mapping_{};
    grid_mapping_type grid_mapping_;

    V buff_;

    /* Constness of mdgrid_buffer can not be deduced outside of member functions.
       So the accessor policies have to be templated. */

    template<bool has_const>
    struct [[nodiscard]]
    element_accessor_policy {
        std::size_t grid_offset;
        std::size_t grid_required_span_size;

        using element_type =
            std::conditional_t<has_const, const element_element_type, element_element_type>;
        using data_handle_type =
            std::conditional_t<has_const, typename V::const_pointer, typename V::pointer>;
        using reference =
            std::conditional_t<has_const, typename V::const_reference, typename V::reference>;
        using offset_policy = element_accessor_policy;

        constexpr reference access(this const auto& self,
                                   data_handle_type const buff_ptr,
                                   const std::size_t element_offset) {
            return buff_ptr[(element_offset * self.grid_required_span_size) + self.grid_offset];
        }

        constexpr offset_policy::data_handle_type offset(this const auto& self,
                                                         data_handle_type const buff_ptr,
                                                         const std::size_t element_offset) {
            return buff_ptr[element_offset * self.grid_required_span_size];
        }
    };

    template<bool has_const>
    using element_mdspan = std::mdspan<typename element_accessor_policy<has_const>::element_type,
                                       ElemExtents,
                                       ElemLP,
                                       element_accessor_policy<has_const>>;

    template<bool has_const>
    struct [[nodiscard]]
    grid_accessor_policy {
        std::size_t grid_required_span_size;

        using element_type = element_mdspan<has_const>;
        struct data_handle_type {
            using buff_ptr_type =
                std::conditional_t<has_const, typename V::const_pointer, typename V::pointer>;

            buff_ptr_type buff_ptr{ nullptr };
            std::size_t grid_offset{ 0uz };
        };
        using reference     = element_type;
        using offset_policy = grid_accessor_policy;

        constexpr reference access(this const auto& self,
                                   data_handle_type const grid_handle,
                                   const std::size_t grid_offset) {
            const auto acc = element_accessor_policy<has_const>{
                .grid_offset             = grid_offset + grid_handle.grid_offset,
                .grid_required_span_size = self.grid_required_span_size
            };
            return element_mdspan<has_const>(grid_handle.buff_ptr, element_mapping_, acc);
        }

        constexpr offset_policy::data_handle_type offset(this const auto&,
                                                         data_handle_type const grid_handle,
                                                         const std::size_t grid_offset) {
            auto new_handle = grid_handle;
            new_handle.grid_offset += grid_offset;
            return new_handle;
        }
    };

  public:
    explicit constexpr mdgrid_buffer(const grid_mapping_type& m)
        : grid_mapping_(m),
          buff_(element_mapping_.required_span_size() * grid_mapping_.required_span_size()) {}

    template<typename Self>
    [[nodiscard]]
    constexpr auto mds(this Self& self) {
        static constexpr bool has_const = std::is_const_v<std::remove_reference_t<Self>>;

        using grid_acc = grid_accessor_policy<has_const>;
        using grid_mdspan =
            std::mdspan<typename grid_acc::element_type, GridExtents, GridLP, grid_acc>;

        return grid_mdspan(
            typename grid_acc::data_handle_type{ .buff_ptr{ self.buff_.data() } },
            self.grid_mapping_,
            grid_acc{ .grid_required_span_size = self.grid_mapping_.required_span_size() });
    }
};

} // namespace tyvi
