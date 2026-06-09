// Copyright 2025 - 2026, Miro Palmu and the tyvi contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

#include "thrust/copy.h"
#include "thrust/device_ptr.h"

#include "tyvi/mdspan.h"

namespace tyvi {

template<typename T,
         std::size_t SegmentSize,
         typename E,
         typename LP        = std::layout_right,
         typename Allocator = std::allocator<T>>
class [[nodiscard]]
mdsegments {
    static_assert(E::rank_dynamic() == 0);

  public:
    using allocator_type   = Allocator;
    using allocator_traits = std::allocator_traits<Allocator>;

    // This assumes that LP::mapping<E> is constexpr constructible.
    static constexpr auto mapping = typename LP::template mapping<E>{};
    static constexpr auto rss     = mapping.required_span_size();

    // TODO: support allocator aware constructors (there is no need atm).

    explicit constexpr mdsegments() = default;
    explicit constexpr mdsegments(std::size_t);

    constexpr ~mdsegments();
    constexpr mdsegments(const mdsegments&) = delete;
    constexpr mdsegments(mdsegments&&) noexcept;
    constexpr mdsegments& operator=(const mdsegments&) = delete;
    constexpr mdsegments& operator=(mdsegments&&) noexcept;

    [[nodiscard]]
    constexpr bool empty() const;
    [[nodiscard]]
    constexpr std::size_t size() const;

    constexpr void resize(std::size_t);

    template<typename U>
    struct [[nodiscard]] inner_accessor_policy;

    template<typename U>
    struct [[nodiscard]] outer_accessor_policy;

    template<typename U>
    using inner_mds = std::mdspan<U, E, LP, inner_accessor_policy<U>>;

    template<typename U>
    using outer_mds = std::mdspan<inner_mds<U>,
                                  std::extents<std::size_t, std::dynamic_extent>,
                                  std::layout_right,
                                  outer_accessor_policy<U>>;

    [[nodiscard]]
    constexpr outer_mds<T> mds();

    [[nodiscard]]
    constexpr outer_mds<const T> mds() const;

    [[nodiscard]]
    constexpr outer_mds<const T> cmds() const;

    // Making views conform to this complicates them unneccessearly.
    // NOLINTBEGIN{misc-non-private-member-variables-in-classes}

    template<typename U>
    struct [[nodiscard]] raw_view_type : std::ranges::view_interface<raw_view_type<U>> {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = U;
        using size_type         = std::size_t;
        using difference_type   = std::ptrdiff_t;
        using reference         = value_type&;

        class [[nodiscard]] iterator_type;
        using iterator = iterator_type;

        constexpr iterator_type begin() const;
        constexpr iterator_type end() const;

        iterator begin_;
        iterator end_;
    };

    template<typename U, E::index_type... idx>
    struct [[nodiscard]] component_view_type :
        std::ranges::view_interface<component_view_type<U, idx...>> {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = U;
        using size_type         = std::size_t;
        using difference_type   = std::ptrdiff_t;
        using reference         = value_type&;

        class [[nodiscard]] iterator_type;
        using iterator = iterator_type;

        constexpr iterator begin() const;
        constexpr iterator end() const;

        iterator begin_;
        iterator end_;
    };

    // NOLINTEND{misc-non-private-member-variables-in-classes}

    constexpr raw_view_type<T> raw_view();

    // Somehow this is triggered even if there is the attribute on the type itself.
    // NOLINTBEGIN{modernize-use-nodiscard}
    constexpr raw_view_type<const T> raw_view() const;
    constexpr raw_view_type<const T> raw_cview() const;
    // NOLINTEND{modernize-use-nodiscard}

    template<E::index_type... idx>
    constexpr component_view_type<T, idx...> component_view();

    template<std::array<typename E::index_type, E::rank()> idx>
    constexpr auto component_view(); // using just auto for simplicity

    template<E::index_type... idx>
    constexpr component_view_type<const T, idx...> component_cview() const;

    template<std::array<typename E::index_type, E::rank()> idx>
    constexpr auto component_cview() const; // using just auto for simplicity

    template<typename t, std::size_t sg, typename e, typename lp, typename a, typename b>
    friend constexpr void h2d_copy(const mdsegments<t, sg, e, lp, a>&,
                                   mdsegments<t, sg, e, lp, b>&);

    template<typename t, std::size_t sg, typename e, typename lp, typename a, typename b>
    friend constexpr void d2h_copy(const mdsegments<t, sg, e, lp, a>&,
                                   mdsegments<t, sg, e, lp, b>&);

  private:
    using allocator_value_type = typename allocator_traits::value_type;
    using allocator_pointer    = typename allocator_traits::pointer;

    using segment_ptr_allocator = allocator_traits::template rebind_alloc<allocator_pointer>;
    using segment_ptr_allocator_traits =
        allocator_traits::template rebind_traits<allocator_pointer>;

    std::vector<allocator_pointer> segments_;
    segment_ptr_allocator_traits::pointer segment_ptrs_{};
    std::size_t number_of_segments_{};
    std::vector<std::size_t> segment_allocation_sizes_;
    constexpr void calculate_segment_allocation_sizes();
    std::size_t outer_size_{};

    [[no_unique_address]]
    Allocator allocator_{};
    [[no_unique_address]]
    segment_ptr_allocator segment_ptr_allocator_{ allocator_ };

    constexpr void free_memory();
};

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr void
mdsegments<T, SG, E, LP, A>::calculate_segment_allocation_sizes() {
    const auto n = std::ranges::max(this->number_of_segments_,
                                    std::reduce(this->segment_allocation_sizes_.begin(),
                                                this->segment_allocation_sizes_.end()));
    if (n == 0uz) {
        this->segment_allocation_sizes_.clear();
        return;
    }
    if (n >= 1uz) {
        this->segment_allocation_sizes_.resize(1uz);
        this->segment_allocation_sizes_.at(0uz) = 1uz;
    }
    if (n >= 2uz) {
        this->segment_allocation_sizes_.resize(2uz);
        this->segment_allocation_sizes_.at(1uz) = 1uz;
    }

    auto sum =
        std::reduce(this->segment_allocation_sizes_.begin(), this->segment_allocation_sizes_.end());

    for (auto i = 2uz; sum < n; ++i) {
        const auto a = this->segment_allocation_sizes_.at(i - 2uz);
        const auto b = this->segment_allocation_sizes_.at(i - 1uz);
        const auto c = a + b;
        this->segment_allocation_sizes_.push_back(c);
        sum += c;
    }
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U>
struct mdsegments<T, SG, E, LP, A>::inner_accessor_policy {
    using element_type     = U;
    using data_handle_type = mdsegments::raw_view_type<U>::iterator;
    using reference        = U&;
    using offset_policy    = mdsegments::inner_accessor_policy<U>;

    [[nodiscard]]
    constexpr reference access(data_handle_type const h, const std::size_t offset) const {
        return *std::ranges::next(h, static_cast<std::ptrdiff_t>(offset * SG));
    }

    [[nodiscard]]
    constexpr offset_policy::data_handle_type offsest(data_handle_type const h,
                                                      const std::size_t offset) const {
        return std::ranges::next(h, static_cast<std::ptrdiff_t>(offset * SG));
    }
};

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U>
struct mdsegments<T, SG, E, LP, A>::outer_accessor_policy {
    using element_type = mdsegments::inner_mds<U>;
    using data_handle_type =
        std::tuple<typename mdsegments::raw_view_type<U>::iterator, std::size_t>;
    using reference     = element_type;
    using offset_policy = mdsegments::outer_accessor_policy<U>;

    [[nodiscard]]
    constexpr reference access(data_handle_type const h, const std::size_t offset) const {
        const auto b         = std::get<0>(h);
        const auto n         = offset + std::get<1>(h);
        const auto segments  = n / SG;
        const auto left_over = n % SG;
        const auto skip      = segments * SG * rss + left_over;
        return element_type(std::ranges::next(b, static_cast<std::ptrdiff_t>(skip)));
    }

    [[nodiscard]]
    constexpr offset_policy::data_handle_type offsest(data_handle_type const h,
                                                      const std::size_t offset) const {
        return { std::get<0>(h), std::get<1>(h) + offset };
    }
};

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr void
mdsegments<T, SG, E, LP, A>::free_memory() {
    segment_ptr_allocator_traits::deallocate(this->segment_ptr_allocator_,
                                             this->segment_ptrs_,
                                             this->number_of_segments_);

    for (const auto& [p, n] : std::views::zip(this->segments_, this->segment_allocation_sizes_)) {
        allocator_traits::deallocate(this->allocator_, p, rss * SG * n);
    }

    this->segments_.clear();
    this->segment_allocation_sizes_.clear();
    this->number_of_segments_ = 0;
    this->outer_size_         = 0;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::mds() -> outer_mds<T> {
    return mdsegments::outer_mds<T>({ this->raw_view().begin(), 0uz }, this->size());
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::cmds() const -> outer_mds<const T> {
    return mdsegments::outer_mds<const T>({ this->raw_view().begin(), 0uz }, this->size());
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::mds() const -> outer_mds<const T> {
    return this->cmds();
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr bool
mdsegments<T, SG, E, LP, A>::empty() const {
    return this->size() == 0;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr std::size_t
mdsegments<T, SG, E, LP, A>::size() const {
    return this->outer_size_;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr void
mdsegments<T, SG, E, LP, A>::resize(const std::size_t outer_size) {
    this->outer_size_ = outer_size;

    const auto required_segments       = (outer_size > 0uz) ? (outer_size - 1uz) / SG + 1uz : 0uz;
    const auto prev_number_of_segments = this->number_of_segments_;
    this->number_of_segments_          = required_segments;
    this->calculate_segment_allocation_sizes();

    const auto need_more_allocations =
        this->segment_allocation_sizes_.size() > this->segments_.size();

    if (need_more_allocations) {
        for (const auto N :
             this->segment_allocation_sizes_ | std::views::drop(this->segments_.size())) {
            this->segments_.push_back(allocator_traits::allocate(this->allocator_, SG * rss * N));
        }
    }

    const auto recompute_ptrs = prev_number_of_segments != this->number_of_segments_;
    if (recompute_ptrs) {
        if (prev_number_of_segments > 0uz) {
            // These will be recomputed...
            segment_ptr_allocator_traits::deallocate(this->segment_ptr_allocator_,
                                                     this->segment_ptrs_,
                                                     prev_number_of_segments);
        }

        this->segment_ptrs_ = segment_ptr_allocator_traits::allocate(this->segment_ptr_allocator_,
                                                                     this->number_of_segments_);

        auto next_allocation           = 0uz;
        auto offset_to_next_allocation = 0uz;

        for (auto i = 0uz; i < this->number_of_segments_; ++i) {
            // ...here.
            segment_ptr_allocator_traits::construct(
                this->segment_ptr_allocator_,
                std::ranges::next(this->segment_ptrs_, static_cast<std::ptrdiff_t>(i)),
                std::ranges::next(
                    this->segments_.at(next_allocation),
                    static_cast<std::ptrdiff_t>(SG * rss * offset_to_next_allocation)));

            if (++offset_to_next_allocation
                >= this->segment_allocation_sizes_.at(next_allocation)) {
                next_allocation += 1uz;
                offset_to_next_allocation = 0uz;
            }
        }
    }
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U>
constexpr auto
mdsegments<T, SG, E, LP, A>::raw_view_type<U>::begin() const -> iterator {
    return this->begin_;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U>
constexpr auto
mdsegments<T, SG, E, LP, A>::raw_view_type<U>::end() const -> iterator {
    return this->end_;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U, E::index_type... idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_view_type<U, idx...>::begin() const -> iterator {
    return this->begin_;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U, E::index_type... idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_view_type<U, idx...>::end() const -> iterator {
    return this->end_;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr mdsegments<T, SG, E, LP, A>::mdsegments(const std::size_t n) : mdsegments() {
    this->resize(n);
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U>
class mdsegments<T, SG, E, LP, A>::raw_view_type<U>::iterator_type {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = mdsegments<T, SG, E, LP, A>::raw_view_type<U>::difference_type;
    using value_type        = mdsegments::allocator_value_type;
    using reference         = mdsegments::allocator_value_type&;
    using pointer           = mdsegments::allocator_pointer;

    constexpr iterator_type() = default;
    constexpr iterator_type(mdsegments::segment_ptr_allocator_traits::pointer const ptr,
                            const std::size_t n)
        : ptr_{ ptr },
          offset_{ n } {}

    /// prefix increment
    constexpr iterator_type& operator++() {
        ++offset_;
        return *this;
    }

    /// postfix increment
    [[nodiscard]]
    constexpr iterator_type operator++(int) {
        iterator_type old = *this;
        ++(*this);
        return old;
    }

    /// prefix decrement
    constexpr iterator_type& operator--() {
        --offset_;
        return *this;
    }

    /// postfix decrement
    [[nodiscard]]
    constexpr iterator_type operator--(int) {
        iterator_type old = *this;
        --(*this);
        return old;
    }

    [[nodiscard]]
    constexpr reference operator*() const {
        const auto segment   = this->offset_ / (rss * SG);
        const auto left_over = this->offset_ % (rss * SG);

        return this->ptr_[segment][left_over];
    }

    [[nodiscard]]
    constexpr auto operator<=>(const iterator_type& rhs) const {
        return this->offset_ <=> rhs.offset_;
    }

    [[nodiscard]]
    constexpr bool operator==(const iterator_type& rhs) const {
        return (this->ptr_ == rhs.ptr_) and (this->offset_ == rhs.offset_);
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const iterator_type& lhs, const iterator_type& rhs) {
        if (lhs.offset_ >= rhs.offset_) {
            return static_cast<difference_type>(lhs.offset_ - rhs.offset_);
        }
        return -static_cast<difference_type>(rhs.offset_ - lhs.offset_);
    }

    [[nodiscard]]
    friend constexpr iterator_type operator+(const iterator_type& lhs, const difference_type rhs) {
        auto result    = lhs;
        result.offset_ = static_cast<std::size_t>(static_cast<difference_type>(lhs.offset_) + rhs);
        return result;
    }

    [[nodiscard]]
    friend constexpr iterator_type operator+(const difference_type lhs, const iterator_type& rhs) {
        return rhs + lhs;
    }

    [[nodiscard]]
    friend constexpr iterator_type operator-(const iterator_type& lhs, const difference_type rhs) {
        return lhs + (-rhs);
    }

    constexpr iterator_type& operator+=(const difference_type rhs) { return *this = *this + rhs; }

    constexpr iterator_type& operator-=(const difference_type rhs) { return *this = *this - rhs; }

    [[nodiscard]]
    constexpr reference operator[](const difference_type rhs) const {
        return *(*this + rhs);
    }

  private:
    mdsegments::segment_ptr_allocator_traits::pointer ptr_{ nullptr };
    std::size_t offset_{ 0 };
};

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::raw_view() -> raw_view_type<T> {
    return raw_view_type<T>{
        .begin_ = typename raw_view_type<T>::iterator(this->segment_ptrs_, 0uz),
        .end_   = typename raw_view_type<T>::iterator(this->segment_ptrs_,
                                                    SG * rss * this->number_of_segments_)
    };
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::raw_cview() const -> raw_view_type<const T> {
    return raw_view_type<const T>{
        .begin_ = typename raw_view_type<const T>::iterator(this->segment_ptrs_, 0uz),
        .end_   = typename raw_view_type<const T>::iterator(this->segment_ptrs_,
                                                          SG * rss * this->number_of_segments_)
    };
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::raw_view() const -> raw_view_type<const T> {
    return this->raw_cview();
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<typename U, E::index_type... idx>
class mdsegments<T, SG, E, LP, A>::component_view_type<U, idx...>::iterator_type {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = mdsegments<T, SG, E, LP, A>::raw_view_type<U>::difference_type;
    using value_type        = mdsegments::allocator_value_type;
    using reference         = mdsegments::allocator_value_type&;
    using pointer           = mdsegments::allocator_pointer;

    constexpr iterator_type() = default;
    constexpr iterator_type(mdsegments::segment_ptr_allocator_traits::pointer const ptr,
                            const std::size_t n)
        : ptr_{ ptr },
          offset_{ n } {}

    /// prefix increment
    constexpr iterator_type& operator++() {
        ++offset_;
        return *this;
    }

    /// postfix increment
    [[nodiscard]]
    constexpr iterator_type operator++(int) {
        iterator_type old = *this;
        ++(*this);
        return old;
    }

    /// prefix decrement
    constexpr iterator_type& operator--() {
        --offset_;
        return *this;
    }

    /// postfix decrement
    [[nodiscard]]
    constexpr iterator_type operator--(int) {
        iterator_type old = *this;
        --(*this);
        return old;
    }

    [[nodiscard]]
    constexpr reference operator*() const {
        const auto segment   = this->offset_ / SG;
        const auto left_over = this->offset_ % SG;

        const auto component_offset_in_segment = rss * mapping(idx...);

        return this->ptr_[segment][component_offset_in_segment + left_over];
    }

    [[nodiscard]]
    constexpr auto operator<=>(const iterator_type& rhs) const {
        return this->offset_ <=> rhs.offset_;
    }

    [[nodiscard]]
    constexpr bool operator==(const iterator_type& rhs) const {
        return (this->ptr_ == rhs.ptr_) and (this->offset_ == rhs.offset_);
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const iterator_type& lhs, const iterator_type& rhs) {
        if (lhs.offset_ >= rhs.offset_) {
            return static_cast<difference_type>(lhs.offset_ - rhs.offset_);
        }
        return -static_cast<difference_type>(rhs.offset_ - lhs.offset_);
    }

    [[nodiscard]]
    friend constexpr iterator_type operator+(const iterator_type& lhs, const difference_type rhs) {
        auto result    = lhs;
        result.offset_ = static_cast<std::size_t>(static_cast<difference_type>(lhs.offset_) + rhs);
        return result;
    }

    [[nodiscard]]
    friend constexpr iterator_type operator+(const difference_type lhs, const iterator_type& rhs) {
        return rhs + lhs;
    }

    [[nodiscard]]
    friend constexpr iterator_type operator-(const iterator_type& lhs, const difference_type rhs) {
        return lhs + (-rhs);
    }

    constexpr iterator_type& operator+=(const difference_type rhs) { return *this = *this + rhs; }

    constexpr iterator_type& operator-=(const difference_type rhs) { return *this = *this - rhs; }

    [[nodiscard]]
    constexpr reference operator[](const difference_type rhs) const {
        return *(*this + rhs);
    }

  private:
    mdsegments::segment_ptr_allocator_traits::pointer ptr_{ nullptr };
    std::size_t offset_{ 0 };
};

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<E::index_type... idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_view() -> component_view_type<T, idx...> {
    return component_view_type<T, idx...>{
        .begin_ = typename component_view_type<T, idx...>::iterator(this->segment_ptrs_, 0uz),
        .end_ = typename component_view_type<T, idx...>::iterator(this->segment_ptrs_, this->size())
    };
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<std::array<typename E::index_type, E::rank()> idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_view() {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return this->component_view<idx[I]...>();
    }(std::make_index_sequence<E::rank()>());
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<E::index_type... idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_cview() const -> component_view_type<const T, idx...> {
    return component_view_type<const T, idx...>{
        .begin_ = typename component_view_type<const T, idx...>::iterator(this->segment_ptrs_, 0uz),
        .end_   = typename component_view_type<const T, idx...>::iterator(this->segment_ptrs_,
                                                                        this->size())
    };
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
template<std::array<typename E::index_type, E::rank()> idx>
constexpr auto
mdsegments<T, SG, E, LP, A>::component_cview() const {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return this->component_cview<idx[I]...>();
    }(std::make_index_sequence<E::rank()>());
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr mdsegments<T, SG, E, LP, A>::~mdsegments() {
    if (this->segments_.empty()) { return; }
    free_memory();
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr mdsegments<T, SG, E, LP, A>::mdsegments(mdsegments&& other) noexcept {
    *this = std::move(other);
}

template<typename T, std::size_t SG, typename E, typename LP, typename A>
constexpr auto
mdsegments<T, SG, E, LP, A>::operator=(mdsegments&& other) noexcept -> mdsegments& {
    if (not this->segments_.empty()) { this->free_memory(); }

    this->segments_ = std::move(other.segments_);
    other.segments_.clear();
    this->segment_ptrs_             = std::move(other.segment_ptrs_);
    this->number_of_segments_       = other.number_of_segments_;
    other.number_of_segments_       = 0;
    this->segment_allocation_sizes_ = std::move(other.segment_allocation_sizes_);
    other.segment_allocation_sizes_.clear();
    this->outer_size_            = other.outer_size_;
    other.outer_size_            = 0;
    this->allocator_             = std::move(other.allocator_);
    this->segment_ptr_allocator_ = std::move(other.segment_ptr_allocator_);

    return *this;
}

template<typename T, std::size_t SG, typename E, typename LP, typename A, typename B>
constexpr void
h2d_copy(const mdsegments<T, SG, E, LP, A>& h, mdsegments<T, SG, E, LP, B>& d) {
    if (h.size() != d.size()) { d.resize(h.size()); }

    static constexpr auto rss = mdsegments<T, SG, E, LP, A>::rss;

    for (const auto [h_ptr, d_ptr] : std::views::zip(h.segments_, d.segments_)) {
        const auto b    = h_ptr;
        const auto e    = std::ranges::next(b, static_cast<std::ptrdiff_t>(SG * rss));
        const auto dest = thrust::device_pointer_cast(d_ptr);
        std::ignore     = thrust::copy(b, e, dest);
    }
}

template<typename T, std::size_t SG, typename E, typename LP, typename A, typename B>
constexpr void
d2h_copy(const mdsegments<T, SG, E, LP, A>& d, mdsegments<T, SG, E, LP, B>& h) {
    if (d.size() != h.size()) { h.resize(d.size()); }

    static constexpr auto rss = mdsegments<T, SG, E, LP, A>::rss;

    for (const auto [h_ptr, d_ptr] : std::views::zip(h.segments_, d.segments_)) {
        const auto b    = thrust::device_pointer_cast(d_ptr);
        const auto e    = std::ranges::next(b, static_cast<std::ptrdiff_t>(SG * rss));
        const auto dest = h_ptr;
        std::ignore     = thrust::copy(b, e, dest);
    }
}
} // namespace tyvi
