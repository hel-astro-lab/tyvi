#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <utility>

#include "thrust/memory.h"

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

    // Making accessor conform to this complicates them unneccessearly.
    // NOLINTBEGIN{misc-non-private-member-variables-in-classes}

    template<bool has_const>
    struct [[nodiscard]]
    element_accessor_policy {
        /// Element accessor has to know about which element element in the grid it accesses.
        std::size_t grid_offset;
        /// Element accessor has to know how many elements there are in the grid.
        std::size_t grid_required_span_size;

        using element_type =
            std::conditional_t<has_const, const element_element_type, element_element_type>;
        using data_handle_type =
            std::conditional_t<has_const, typename V::const_pointer, typename V::pointer>;
        using reference =
            std::conditional_t<has_const, typename V::const_reference, typename V::reference>;
        using offset_policy = element_accessor_policy;

        [[nodiscard]]
        constexpr reference access(data_handle_type const buff_ptr,
                                   const std::size_t element_offset) const {
            return buff_ptr[static_cast<std::ptrdiff_t>((element_offset * grid_required_span_size)
                                                        + grid_offset)];
        }

        [[nodiscard]]
        constexpr offset_policy::data_handle_type offset(data_handle_type const buff_ptr,
                                                         const std::size_t element_offset) const {
            return std::ranges::next(buff_ptr, element_offset * grid_required_span_size);
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

        [[nodiscard]]
        constexpr reference access(data_handle_type const grid_handle,
                                   const std::size_t grid_offset) const {
            const auto acc = element_accessor_policy<has_const>{
                .grid_offset             = grid_offset + grid_handle.grid_offset,
                .grid_required_span_size = grid_required_span_size
            };
            return element_mdspan<has_const>(grid_handle.buff_ptr, element_mapping_, acc);
        }

        [[nodiscard]]
        constexpr offset_policy::data_handle_type offset(data_handle_type const grid_handle,
                                                         const std::size_t grid_offset) const {
            auto new_handle = grid_handle;
            new_handle.grid_offset += grid_offset;
            return new_handle;
        }
    };

    // NOLINTEND{misc-non-private-member-variables-in-classes}

  public:
    explicit constexpr mdgrid_buffer(const grid_mapping_type& m)
        : grid_mapping_(m),
          buff_(element_mapping_.required_span_size() * grid_mapping_.required_span_size()) {}

    template<typename... Indices>
        requires std::constructible_from<GridExtents, Indices...>
    explicit constexpr mdgrid_buffer(Indices... indices)
        : mdgrid_buffer(grid_mapping_type(GridExtents{ std::forward<Indices>(indices)... })) {}

    explicit constexpr mdgrid_buffer(const GridExtents& extents)
        : mdgrid_buffer(grid_mapping_type(extents)) {}

    constexpr void invalidating_resize(const GridExtents& extents) {
        this->grid_mapping_ = grid_mapping_type(extents);
        buff_.resize(element_mapping_.required_span_size() * grid_mapping_.required_span_size());
    }

    template<typename... Indices>
        requires std::constructible_from<GridExtents, Indices...>
    constexpr void invalidating_resize(Indices... indices) {
        this->invalidating_resize(GridExtents{ std::forward<Indices>(indices)... });
    }

    /// Get copy of the underlying buffer.
    [[nodiscard]]
    constexpr V underlying_buffer() const {
        return buff_;
    }

    /// Set data in the underlying buffer.
    ///
    /// Invalidates any pointers to the underlying buffer.
    /// This includes pointers in [md]spans.
    ///
    /// Throws std::invalid_argument if given buffer is not the same length
    /// as the current underlying buffer.
    template<std::ranges::sized_range T>
        requires std::assignable_from<V&, T>
    constexpr void set_underlying_buffer(T&& other) {
        const auto my_size    = std::ranges::size(buff_);
        const auto other_size = std::ranges::size(other);

        if (my_size != other_size) {
            throw std::invalid_argument{
                std::format("Expected {} sized buffer, got: {}", my_size, other_size)
            };
        }

        buff_ = std::forward<T>(other);
    }

    [[nodiscard]]
    constexpr auto mds() {
        static constexpr bool has_const = false;
        using grid_acc                  = grid_accessor_policy<has_const>;
        using grid_mdspan =
            std::mdspan<typename grid_acc::element_type, GridExtents, GridLP, grid_acc>;

        return grid_mdspan(
            typename grid_acc::data_handle_type{ .buff_ptr{ buff_.data() } },
            grid_mapping_,
            grid_acc{ .grid_required_span_size = grid_mapping_.required_span_size() });
    }

    [[nodiscard]]
    constexpr auto mds() const {
        static constexpr bool has_const = true;
        using grid_acc                  = grid_accessor_policy<has_const>;
        using grid_mdspan =
            std::mdspan<typename grid_acc::element_type, GridExtents, GridLP, grid_acc>;

        return grid_mdspan(
            typename grid_acc::data_handle_type{ .buff_ptr{ buff_.data() } },
            grid_mapping_,
            grid_acc{ .grid_required_span_size = grid_mapping_.required_span_size() });
    }

    /// Iterate over all grid points and components at each grid point.
    ///
    /// Iteration order is not specified but such that sequential values are
    /// stored contiquously in memory.
    [[nodiscard]]
    constexpr auto begin() {
        return std::ranges::begin(buff_);
    }

    [[nodiscard]]
    constexpr auto begin() const {
        return std::ranges::begin(buff_);
    }

    [[nodiscard]]
    constexpr auto end() {
        return std::ranges::end(buff_);
    }

    [[nodiscard]]
    constexpr auto end() const {
        return std::ranges::end(buff_);
    }

    [[nodiscard]]
    constexpr auto grid_extents() const {
        return grid_mapping_.extents();
    }

    [[nodiscard]]
    constexpr auto element_extents() const {
        return element_mapping_.extents();
    }

    /// Span of the underlying data buffer.
    [[nodiscard]]
    constexpr auto span() {
        static constexpr bool has_const = false;

        using span_value_type =
            std::conditional_t<has_const, const element_element_type, element_element_type>;
        using S = std::span<span_value_type>;

        // Somewhat a hack, but required to support thrust vectors.
        if constexpr (requires(V v) { std::ranges::data(v); }) {
            return S(std::ranges::data(buff_), std::ranges::size(buff_));
        } else {
            return S(thrust::raw_pointer_cast(buff_.data()), std::ranges::size(buff_));
        }
    }

    /// Span of the underlying data buffer.
    [[nodiscard]]
    constexpr auto span() const {
        static constexpr bool has_const = true;

        using span_value_type =
            std::conditional_t<has_const, const element_element_type, element_element_type>;
        using S = std::span<span_value_type>;

        // Somewhat a hack, but required to support thrust vectors.
        if constexpr (requires(V v) { std::ranges::data(v); }) {
            return S(std::ranges::data(buff_), std::ranges::size(buff_));
        } else {
            return S(thrust::raw_pointer_cast(buff_.data()), std::ranges::size(buff_));
        }
    }
};

} // namespace tyvi
