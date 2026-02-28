#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace tyvi {

/// CPU-side contiguous container with over-allocation strategy.
///
/// Modernized ManVec with C++23 type traits. Intended as a drop-in
/// replacement for thrust::device_vector on the CPU backend.
template<typename T>
    requires std::is_trivially_copyable_v<T>
class dynamic_array {
  public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator        = T*;
    using const_iterator  = const T*;

  private:
    pointer ptr_{ nullptr };
    size_type size_{ 0 };
    size_type capacity_{ 0 };

    static constexpr float over_alloc_factor_ = 1.5f;

    void reallocate(size_type new_cap) {
        auto* new_ptr      = new T[new_cap];
        const auto to_copy = std::min(size_, new_cap);
        if (ptr_ && to_copy > 0) { std::memcpy(new_ptr, ptr_, sizeof(T) * to_copy); }
        delete[] ptr_;
        ptr_      = new_ptr;
        capacity_ = new_cap;
    }

  public:
    dynamic_array() = default;

    explicit dynamic_array(size_type count)
        : ptr_(new T[count]),
          size_(count),
          capacity_(count) {}

    template<std::input_iterator Iter>
        requires(!std::same_as<std::remove_cvref_t<Iter>, size_type>)
    dynamic_array(Iter first, Iter last)
        : ptr_(nullptr),
          size_(0),
          capacity_(0) {
        if constexpr (std::random_access_iterator<Iter>) {
            const auto n = static_cast<size_type>(std::distance(first, last));
            if (n > 0) {
                ptr_      = new T[n];
                size_     = n;
                capacity_ = n;
                std::copy(first, last, ptr_);
            }
        } else {
            for (; first != last; ++first) { push_back(*first); }
        }
    }

    ~dynamic_array() { delete[] ptr_; }

    // copy constructor
    dynamic_array(const dynamic_array& other)
        : ptr_(other.capacity_ ? new T[other.capacity_] : nullptr),
          size_(other.size_),
          capacity_(other.capacity_) {
        if (size_ > 0) { std::memcpy(ptr_, other.ptr_, sizeof(T) * size_); }
    }

    // move constructor
    dynamic_array(dynamic_array&& other) noexcept
        : ptr_(other.ptr_),
          size_(other.size_),
          capacity_(other.capacity_) {
        other.ptr_      = nullptr;
        other.size_     = 0;
        other.capacity_ = 0;
    }

    // copy assignment (copy-and-swap for exception safety)
    dynamic_array& operator=(const dynamic_array& other) {
        if (this != &other) {
            auto tmp = other;
            swap(*this, tmp);
        }
        return *this;
    }

    friend void swap(dynamic_array& a, dynamic_array& b) noexcept {
        std::swap(a.ptr_, b.ptr_);
        std::swap(a.size_, b.size_);
        std::swap(a.capacity_, b.capacity_);
    }

    // move assignment
    dynamic_array& operator=(dynamic_array&& other) noexcept {
        if (this != &other) {
            delete[] ptr_;
            ptr_            = other.ptr_;
            size_           = other.size_;
            capacity_       = other.capacity_;
            other.ptr_      = nullptr;
            other.size_     = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void push_back(const T& val) {
        if (size_ >= capacity_) {
            reallocate(static_cast<size_type>(
                static_cast<float>(capacity_ + 1) * over_alloc_factor_));
        }
        ptr_[size_++] = val;
    }

    void resize(size_type new_size) {
        if (new_size > capacity_) { reallocate(new_size); }
        size_ = new_size;
    }

    void reserve(size_type new_cap) {
        if (new_cap > capacity_) { reallocate(new_cap); }
    }

    void shrink_to_fit() {
        if (capacity_ > size_) { reallocate(size_); }
    }

    void clear() { size_ = 0; }

    template<std::input_iterator Iter>
    void assign(Iter first, Iter last) {
        if constexpr (std::random_access_iterator<Iter>) {
            const auto n = static_cast<size_type>(std::distance(first, last));
            resize(n);
            std::copy(first, last, ptr_);
        } else {
            clear();
            for (; first != last; ++first) { push_back(*first); }
        }
    }

    void set_size(size_type new_size) { size_ = new_size; }

    [[nodiscard]]
    reference operator[](size_type idx) {
        return ptr_[idx];
    }

    [[nodiscard]]
    const_reference operator[](size_type idx) const {
        return ptr_[idx];
    }

    [[nodiscard]]
    pointer data() noexcept {
        return ptr_;
    }

    [[nodiscard]]
    const_pointer data() const noexcept {
        return ptr_;
    }

    [[nodiscard]]
    size_type size() const noexcept {
        return size_;
    }

    [[nodiscard]]
    size_type capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]]
    iterator begin() noexcept {
        return ptr_;
    }

    [[nodiscard]]
    iterator end() noexcept {
        return ptr_ + size_;
    }

    [[nodiscard]]
    const_iterator begin() const noexcept {
        return ptr_;
    }

    [[nodiscard]]
    const_iterator end() const noexcept {
        return ptr_ + size_;
    }

    [[nodiscard]]
    const_iterator cbegin() const noexcept {
        return ptr_;
    }

    [[nodiscard]]
    const_iterator cend() const noexcept {
        return ptr_ + size_;
    }

    [[nodiscard]]
    friend bool operator==(const dynamic_array& a, const dynamic_array& b) {
        return std::ranges::equal(a, b);
    }

};

} // namespace tyvi
