#pragma once

#include <array>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <type_traits>
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

template<std::size_t rank, std::size_t D>
[[nodiscard]]
consteval auto
geometric_index_space() {
    namespace rv = std::ranges::views;

    return std::invoke(
        []<std::size_t... I>(std::index_sequence<I...>) {
            return rv::cartesian_product(rv::iota(I * 0uz, D)...)
                   | std::views::transform([](const auto& t) {
                         return std::apply(
                             [](const auto... i) { return std::array<std::size_t, rank>{ i... }; },
                             t);
                     });
        },
        std::make_index_sequence<rank>());
};

template<typename M>
concept iterable_strided_mapping = (M::is_always_exhaustive()) and requires(M m, std::size_t i) {
    typename M::index_type;
    { m.stride(i) } -> std::same_as<typename M::index_type>;
};

/// Iterate over index space of given mapping.
///
/// Returned view assumes that the given mapping is within its lifetime.
template<typename M>
constexpr std::ranges::view auto
index_space(const M& mapping) {
    static constexpr auto rank = M::extents_type::rank();
    class index_space_iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::array<typename M::index_type, rank>;
        /// This iterator produces values, so our reference is also a value.
        using reference = value_type;
        /// Iterator does not point anything, so pointer doesn't make sense.
        using pointer = void;

      private:
        std::size_t offset_{ std::dynamic_extent };
        const M* mapping_{ nullptr };

      public:
        explicit constexpr index_space_iterator(const std::size_t i, const M* const m)
            : offset_{ i },
              mapping_(m) {}

        explicit constexpr index_space_iterator() = default;

        /// prefix increment
        constexpr index_space_iterator& operator++() {
            ++offset_;
            return *this;
        }

        /// postfix increment
        [[nodiscard]]
        constexpr index_space_iterator operator++(int) {
            index_space_iterator old = *this;
            ++(*this);
            return old;
        }

        [[nodiscard]]
        constexpr reference operator*() const {
            static_assert(rank == 0 or iterable_strided_mapping<M>,
                          "Index space is currently supported only for strided layout mappings.");
            if constexpr (rank == 0) {
                return value_type{};
            } else /* if constexpr (iterable_strided_mapping<M>) */ {
                return [&]<std::size_t... I>(std::index_sequence<I...>) {
                    return std::array{ (offset_ / mapping_->stride(I))
                                       % mapping_->extents().extent(I)... };
                }(std::make_index_sequence<rank>());
            }
        }

        [[nodiscard]]
        constexpr bool operator==(const index_space_iterator&) const = default;
    };

    class [[nodiscard]] index_space_view : public std::ranges::view_interface<index_space_view> {
        const M* mapping_;

      public:
        constexpr explicit index_space_view(const M& m) : mapping_(&m) {}
        [[nodiscard]]
        constexpr auto begin() const {
            return index_space_iterator(0uz, mapping_);
        }
        [[nodiscard]]
        constexpr auto end() const {
            return index_space_iterator(mapping_->required_span_size(), mapping_);
        }
    };

    return index_space_view(mapping);
}

/// Iterate over index space of given std::mdspan.
///
/// Returned view assumes that the given std::mdspan is within its lifetime.
template<typename T, typename E, typename LP, typename AP>
constexpr std::ranges::view auto
index_space(const std::mdspan<T, E, LP, AP>& mds) {
    return index_space(mds.mapping());
};

} // namespace tyvi::sstd
