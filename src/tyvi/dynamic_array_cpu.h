#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <ranges>
#include <utility>

namespace tyvi {

/// CPU-side contiguous container with over-allocation strategy.
///
/// Modernized ManVec with C++23 type traits. Intended as a drop-in
/// replacement for thrust::device_vector on the CPU backend.
template<typename T>
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

    static constexpr float over_alloc_factor_ = 1.2f;

    void reallocate(size_type new_cap) {
        auto* new_ptr      = new T[new_cap];
        const auto to_copy = std::min(size_, new_cap);
        if (ptr_ && to_copy > 0) { std::memcpy(new_ptr, ptr_, sizeof(T) * to_copy); }
        delete[] ptr_;
        ptr_      = new_ptr;
        capacity_ = new_cap;
    }

  public:
    dynamic_array() : ptr_(new T[1]), size_(0), capacity_(1) {}

    explicit dynamic_array(size_type count)
        : ptr_(new T[count]),
          size_(count),
          capacity_(count) {}

    ~dynamic_array() { delete[] ptr_; }

    // copy constructor
    dynamic_array(const dynamic_array& other)
        : ptr_(new T[other.capacity_]),
          size_(other.size_),
          capacity_(other.capacity_) {
        std::memcpy(ptr_, other.ptr_, sizeof(T) * size_);
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

    // copy assignment
    dynamic_array& operator=(const dynamic_array& other) {
        if (this != &other) {
            delete[] ptr_;
            capacity_ = other.capacity_;
            size_     = other.size_;
            ptr_      = new T[capacity_];
            std::memcpy(ptr_, other.ptr_, sizeof(T) * size_);
        }
        return *this;
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
        if (size_ < capacity_) {
            ptr_[size_++] = val;
        } else {
            reallocate(static_cast<size_type>(
                static_cast<float>(capacity_ + 1) * over_alloc_factor_));
            push_back(val);
        }
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

    [[nodiscard]]
    friend bool operator!=(const dynamic_array& a, const dynamic_array& b) {
        return !(a == b);
    }
};

} // namespace tyvi
