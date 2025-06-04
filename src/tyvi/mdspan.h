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

template<typename T>
concept random_access_view = std::ranges::view<T> and std::ranges::random_access_range<T>;

/* Some thrust algorithms use unqualified `distance(b, e)` call which is ambiguous,
   because it finds thrust::distance in addition of std::distance.

   std::distance is found becuase of ADL which searches namespaces of type template parameters
   and all standard mapping types are in std namespace with std::distance.

   This workaround to prevents ADL for type template parameters and was found here:
   https://www.reddit.com/r/cpp/comments/rsslxq/how_does_this_for_hiding_a_template_type/ */
namespace {
template<typename ADLMapping>
struct index_space_iterator_impl {
    class index_space_iterator;
};

template<typename ADLMapping>
class index_space_iterator_impl<ADLMapping>::index_space_iterator {
    using M = ADLMapping;

  public:
    static constexpr auto rank = M::extents_type::rank();

    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::array<typename M::index_type, rank>;
    /// This iterator produces values, so our reference is also a value.
    using reference = value_type;
    /// Iterator does not point anything, so pointer doesn't make sense.
    using pointer = void;

  private:
    std::size_t offset_{ std::dynamic_extent };
    /// Mapping has to be stored by value, as it has to be accessible in device code.
    M mapping_{};

  public:
    explicit constexpr index_space_iterator(const std::size_t i, const M& m)
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
        static_assert(rank == 0 or M::is_always_strided(),
                      "Index space is currently supported only for strided layout mappings.");
        if constexpr (rank == 0) {
            return value_type{};
        } else /* if constexpr (M::is_always_strided()) */ {
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return reference{ (offset_ / mapping_.stride(I))
                                  % mapping_.extents().extent(I)... };
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
} // namespace

template<typename Mapping>
using index_space_iterator = typename index_space_iterator_impl<Mapping>::index_space_iterator;

template<typename M>
class [[nodiscard]] index_space_view : public std::ranges::view_interface<index_space_view<M>> {
    M mapping_;

  public:
    constexpr explicit index_space_view(const M& m) : mapping_(m) {}
    [[nodiscard]]
    constexpr auto begin() const {
        return index_space_iterator<M>(0uz, mapping_);
    }
    [[nodiscard]]
    constexpr auto end() const {
        return index_space_iterator<M>(mapping_.required_span_size(), mapping_);
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
