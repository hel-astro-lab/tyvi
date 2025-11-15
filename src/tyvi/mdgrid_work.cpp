#include "tyvi/mdgrid.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

using stream_factory = tyvi::detail::stream_factory;
using stream_handle  = stream_factory::stream_handle;
using stream_t       = stream_factory::stream_t;

void
stream_handle::release_stream() {
    if (this->active_) { this->promise_.set_value(this->stream_); }
    this->active_ = false;
}

stream_handle::stream_handle(stream_t stream, stream_promise promise)
    : active_{ true },
      stream_{ stream },
      promise_{ std::move(promise) } {}

stream_handle::stream_handle(stream_handle&& other) noexcept {
    this->release_stream();

    this->stream_  = other.stream_;
    this->promise_ = std::move(other.promise_);
    this->active_  = other.active_;
    other.active_  = false;
}

stream_handle&
stream_handle::operator=(stream_handle&& other) noexcept {
    this->release_stream();

    this->stream_  = other.stream_;
    this->promise_ = std::move(other.promise_);
    this->active_  = other.active_;
    other.active_  = false;

    return *this;
}

stream_handle::~stream_handle() { this->release_stream(); }

stream_t
stream_handle::get() const {
    if (not active_) { throw std::runtime_error{ "Trying to access inactive stream." }; }
    return stream_;
}

thrust::hip_rocprim::execute_on_stream_nosync
stream_handle::on_stream() const {
    if (not active_) { throw std::runtime_error{ "Trying to use inactive stream." }; }
    return thrust::hip_rocprim::execute_on_stream_nosync{ stream_ };
}

void
stream_handle::wait() const {
    if (active_) { tyvi::detail::hip_check_error(hipStreamSynchronize(stream_)); }
}

stream_handle
stream_factory::get() {
    [[maybe_unused]]
    const std::scoped_lock _{ this->mutex_ };

    const auto p = std::ranges::find_if(managed_streams_, [](stream_future& f) {
        if (not f.valid()) { throw std::logic_error{ "We should not have invalid futures." }; }

        const auto s = f.wait_for(std::chrono::seconds(0));

        if (s == std::future_status::deferred) {
            throw std::logic_error{ "We should not deferred futures." };
        }

        return s == std::future_status::ready;
    });

    stream_t stream{};
    stream_future* future = nullptr;

    if (p == std::ranges::end(managed_streams_)) {
        // Create new stream.
        tyvi::detail::hip_check_error(hipStreamCreateWithFlags(&stream, hipStreamNonBlocking));
        managed_streams_.emplace_back();
        future = &managed_streams_.back();
    } else {
        // Use available stream.
        stream = p->get();
        future = &*p;
    }

    auto promise = stream_promise{};
    *future      = promise.get_future();

    return stream_handle(stream, std::move(promise));
}

stream_factory&
tyvi::detail::global_stream_factory() {
    static stream_factory factory{};
    return factory;
}

tyvi::mdgrid_work::mdgrid_work() : handle_{ tyvi::detail::global_stream_factory().get() } {}
