#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
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

/// Type trait to detect if type is specialization of std::extents.
template<typename>
struct is_mds_extents : std::bool_constant<false> {};

template<typename IndexType, std::size_t... Extents>
struct is_mds_extents<std::extents<IndexType, Extents...>> : std::bool_constant<true> {};

template<typename T>
constexpr bool is_mds_extents_v = is_mds_extents<T>::value;

/// T is specialization of std::extents.
template<typename T>
concept mds_extents = (is_mds_extents_v<T>);

/// mds_extents but every extent is statically known.
template<typename T>
concept static_mds_extents = mds_extents<T> and (T::rank_dynamic() == 0);

/// Convert std::extents<I, E...> to std::array<I, rank>.
template<mds_extents E>
[[nodiscard]]
constexpr auto
as_array(const E& e) -> std::array<typename E::index_type, E::rank()> {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<typename E::index_type, E::rank()>{ e.extent(I)... };
    }(std::make_index_sequence<E::rank()>());
}

/*
/// Convert std::extents<I, E...> to std::array<I, rank> during compile time.
template<typename IndexType, std::size_t... Extents>
    requires static_mds_extents<std::extents<IndexType, Extents...>>
[[nodiscard]]
consteval auto
as_array(std::extents<IndexType, Extents...>) -> std::array<IndexType, sizeof...(Extents)> {
    return std::array{ static_cast<IndexType>(Extents)... };
}
    */

template<typename T>
concept random_access_view = std::ranges::view<T> and std::ranges::random_access_range<T>;

/// Heuristic fo LayoutMapping named requirement.
template<typename M>
concept layout_mapping = requires(M m) {
    typename M::extents_type;
    typename M::index_type;
    typename M::rank_type;
    typename M::layout_type;

    m.extents();
    { m.required_span_size() } -> std::same_as<typename M::index_type>;
    { m.is_unique() } -> std::same_as<bool>;
    { m.is_exhaustive() } -> std::same_as<bool>;
    { m.is_strided() } -> std::same_as<bool>;

    { M::is_always_strided() } -> std::same_as<bool>;
    { M::is_always_exhaustive() } -> std::same_as<bool>;
    { M::is_always_unique() } -> std::same_as<bool>;

    // Make sure these are constant expressions.
    std::bool_constant<M::is_always_strided()>::value;
    std::bool_constant<M::is_always_exhaustive()>::value;
    std::bool_constant<M::is_always_unique()>::value;
};

template<typename M>
concept strided_mapping = layout_mapping<M> and (M::is_always_strided());

template<typename M>
concept unique_mapping = layout_mapping<M> and (M::is_always_unique());

template<typename M>
concept invertable_strided_mapping = strided_mapping<M> and unique_mapping<M>;

template<typename M>
concept exhaustive_mapping = layout_mapping<M> and (M::is_always_exhaustive());

template<typename M>
concept exhaustive_invertable_strided_mapping =
    exhaustive_mapping<M> and invertable_strided_mapping<M>;

template<typename M, std::size_t rank>
concept mapping_of_rank = layout_mapping<M> and (M::extents_type::rank() == rank);

template<typename M>
concept mapping_of_nonzero_rank = layout_mapping<M> and (M::extents_type::rank() > 0uz);

template<std::size_t rank, typename IndexType>
class index_space_iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::array<IndexType, rank>;
    /// This iterator produces values, so our reference is also a value.
    using reference = value_type;
    /// Iterator does not point anything, so pointer doesn't make sense.
    using pointer = void;

  private:
    std::size_t offset_{ std::dynamic_extent };

    // Ideally there could just be a pointer to the index_space_view,
    // but it is possible to have the view live on host and
    // the pointers on device, which makes the scheme impossible.
    //
    // If other than strided mappings are supported this has to be changed.
    std::array<IndexType, rank> dividers_;
    std::array<IndexType, rank> extents_;

  public:
    template<mapping_of_rank<rank> M>
        requires(rank == 0uz)
    explicit constexpr index_space_iterator(const std::size_t offset,
                                            [[maybe_unused]]
                                            const M&)
        : offset_{ offset },
          dividers_{},
          extents_{} {}

    template<mapping_of_rank<rank> M>
        requires exhaustive_invertable_strided_mapping<M> and mapping_of_nonzero_rank<M>
    explicit constexpr index_space_iterator(const std::size_t offset, const M& m)
        : offset_{ offset },
          dividers_{ [&]<std::size_t... I>(std::index_sequence<I...>) {
              return std::array{ m.stride(I)... };
          }(std::make_index_sequence<rank>()) },
          extents_{ as_array(m.extents()) } {}

    template<mapping_of_rank<rank> M>
        requires invertable_strided_mapping<M> and mapping_of_nonzero_rank<M>
    explicit constexpr index_space_iterator(const std::size_t offset, const M& m)
        : offset_{ offset },
          extents_{ as_array(m.extents()) } {
        const auto sorted_rank_ordinals = [&] {
            auto rank_ordinals = []<std::size_t... I>(std::index_sequence<I...>) {
                return std::array{ I... };
            }(std::make_index_sequence<rank>());

            auto as_stride = [&](const std::size_t n) { return m.stride(n); };

            std::ranges::sort(rank_ordinals, {}, as_stride);
            return rank_ordinals;
        }();

        const auto sorted_dividers = [&] {
            auto as_extent = [e = m.extents()](const std::size_t n) { return e.extent(n); };

            auto tmp = std::array<IndexType, rank>{};
            std::transform_exclusive_scan(sorted_rank_ordinals.begin(),
                                          sorted_rank_ordinals.end(),
                                          tmp.begin(),
                                          IndexType{ 1 },
                                          std::multiplies<>{},
                                          as_extent);
            return tmp;
        }();

        for (const auto [i, d] : std::views::enumerate(sorted_dividers)) {
            dividers_.at(sorted_rank_ordinals.at(static_cast<std::size_t>(i))) = d;
        }
    }

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

    /// prefix decrement
    constexpr index_space_iterator& operator--() {
        --offset_;
        return *this;
    }

    /// postfix decrement
    [[nodiscard]]
    constexpr index_space_iterator operator--(int) {
        index_space_iterator old = *this;
        --(*this);
        return old;
    }

    [[nodiscard]]
    constexpr reference operator*() const {
        if constexpr (rank == 0) {
            return value_type{};
        } else {
            // Strided implementation (currently only one supported).
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return reference{ (static_cast<IndexType>(offset_) / dividers_[I])
                                  % extents_[I]... };
            }(std::make_index_sequence<rank>());
        }
    }

    [[nodiscard]]
    constexpr auto operator<=>(const index_space_iterator& rhs) const {
        return this->offset_ <=> rhs.offset_;
    }

    [[nodiscard]]
    constexpr bool operator==(const index_space_iterator& rhs) const {
        return this->offset_ == rhs.offset_;
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const index_space_iterator& lhs,
                                               const index_space_iterator& rhs) {
        if (lhs.offset_ >= rhs.offset_) {
            return static_cast<difference_type>(lhs.offset_ - rhs.offset_);
        }
        return -static_cast<difference_type>(rhs.offset_ - lhs.offset_);
    }

    [[nodiscard]]
    friend constexpr index_space_iterator operator+(const index_space_iterator& lhs,
                                                    const difference_type rhs) {
        auto result    = lhs;
        result.offset_ = static_cast<std::size_t>(static_cast<difference_type>(lhs.offset_) + rhs);
        return result;
    }

    [[nodiscard]]
    friend constexpr index_space_iterator operator+(const difference_type lhs,
                                                    const index_space_iterator& rhs) {
        return rhs + lhs;
    }

    [[nodiscard]]
    friend constexpr index_space_iterator operator-(const index_space_iterator& lhs,
                                                    const difference_type rhs) {
        return lhs + (-rhs);
    }

    constexpr index_space_iterator& operator+=(const difference_type rhs) {
        return *this = *this + rhs;
    }

    constexpr index_space_iterator& operator-=(const difference_type rhs) {
        return *this = *this - rhs;
    }

    [[nodiscard]]
    constexpr reference operator[](const difference_type rhs) const {
        return *(*this + rhs);
    }
};

/*
User defined deduction guides are broken in hip.
see: https://github.com/llvm/llvm-project/issues/146646

template<layout_mapping M>
index_space_iterator(std::size_t, const M&)
    -> index_space_iterator<M::extents_type::rank(), typename M::index_type>;
*/

/// Index space of given mapping ordered based on the corresponding offsets.
///
/// Currently only supports strided mappings.
template<typename M>
class [[nodiscard]] index_space_view : public std::ranges::view_interface<index_space_view<M>> {
    M mapping_;

  public:
    using iterator_type = index_space_iterator<M::extents_type::rank(), typename M::index_type>;

    constexpr explicit index_space_view(const M& m) : mapping_(m) {}

    [[nodiscard]]
    constexpr auto begin() const {
        return iterator_type(0uz, mapping_);
    }
    [[nodiscard]]
    constexpr auto end() const {
        const auto extents = as_array(mapping_.extents());

        const auto last = std::reduce(extents.begin(),
                                      extents.end(),
                                      typename M::index_type{ 1 },
                                      std::multiplies<>{});

        return iterator_type(last, mapping_);
    }
};

/// Iterate over index space of given mapping.
///
/// Returned view assumes that the given mapping is within its lifetime.
template<typename M>
constexpr random_access_view auto
index_space(const M& mapping) {
    return index_space_view<M>(mapping);
}

/// Iterate over index space of given std::mdspan.
///
/// Returned view assumes that the given std::mdspan is within its lifetime.
template<typename T, typename E, typename LP, typename AP>
constexpr random_access_view auto
index_space(const std::mdspan<T, E, LP, AP>& mds) {
    return index_space(mds.mapping());
};

} // namespace tyvi::sstd
