#ifndef CONCURRENT_MVCC_H
#define CONCURRENT_MVCC_H

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

namespace frenzy {

/**
 * Multi-Version Concurrency Control (MVCC) is a basic technique for elimination of starvation.
 * MVCC allows several versions of an object to exist at the same time.
 * That is, there are the "current" version and one or more previous versions.
 * Readers acquire the current version and work with it as much as they want.
 * During that a writer can create and publish a new version of an object, which becomes the current.
 * Readers still work the previous version and can't block/starve writers.
 * When readers end with an old version of an object, it goes away.
 */

#define MVCC_CONTENSION_BACKOFF_SLEEP_MS 50

template <class ValueType>
struct snapshot {
    snapshot(size_t ver) : version{ver}, value{} {}

    template <class U>
    snapshot(size_t ver, U &&arg) : version{ver}, value{std::forward<U>(arg)} {}

    size_t version;
    ValueType value;
};

template <class ValueType>
class mvcc {
public:
    mvcc() : mutable_current_{std::make_shared<snapshot<ValueType>>(0)} {}

    mvcc(ValueType const &value) : mutable_current_{std::make_shared<snapshot<ValueType>>(0, value)} {}
    mvcc(ValueType &&value) : mutable_current_{std::make_shared<snapshot<ValueType>>(0, std::move(value))} {}

    mvcc(mvcc const &other) : mutable_current_{std::atomic_load(other)} {}
    mvcc(mvcc &&other) : mutable_current_{std::atomic_load(other)} {}

    ~mvcc() = default;

    mvcc &operator=(mvcc const &other) {
        std::atomic_store(&this->mutable_current_, std::atomic_load(&other.mutable_current_));
        return *this;
    }
    mvcc &operator=(mvcc &&other) {
        std::atomic_store(&this->mutable_current_, std::atomic_load(&other.mutable_current_));
        return *this;
    }

    std::shared_ptr<snapshot<ValueType> const> current() { return std::atomic_load(&mutable_current_); }

    std::shared_ptr<snapshot<ValueType> const> operator*() { return this->current(); }

    std::shared_ptr<snapshot<ValueType> const> operator->() { return this->current(); }

    std::shared_ptr<snapshot<ValueType> const> overwrite(ValueType const &value) { return this->overwrite_impl(value); }

    std::shared_ptr<snapshot<ValueType> const> overwrite(ValueType &&value) {
        return this->overwrite_impl(std::move(value));
    }

    template <class Updater>
    std::shared_ptr<snapshot<ValueType> const> update(Updater updater);

    template <class Updater>
    std::shared_ptr<snapshot<ValueType> const> try_update(Updater updater) {
        return this->try_update_impl(updater);
    }

    template <class Updater, class Clock, class Duration>
    std::shared_ptr<snapshot<ValueType> const> try_update_until(
        Updater updater, std::chrono::time_point<Clock, Duration> const &timeout_time) {
        return this->try_update_until_impl(updater, timeout_time);
    }

    template <class Updater, class Rep, class Period>
    std::shared_ptr<snapshot<ValueType> const> try_update_for(
        Updater updater, std::chrono::duration<Rep, Period> const &timeout_duration) {
        auto timeout_time = std::chrono::high_resolution_clock::now() + timeout_duration;
        return this->try_update_until_impl(updater, timeout_time);
    }

private:
    template <class U>
    std::shared_ptr<snapshot<ValueType> const> overwrite_impl(U &&value);

    template <class Updater>
    std::shared_ptr<snapshot<ValueType> const> try_update_impl(Updater &updater);

    template <class Updater, class Clock, class Duration>
    std::shared_ptr<snapshot<ValueType> const> try_update_until_impl(
        Updater &updater, std::chrono::time_point<Clock, Duration> const &timeout_time);

private:
    std::shared_ptr<snapshot<ValueType>> mutable_current_;
};

template <class ValueType>
template <class U>
auto mvcc<ValueType>::overwrite_impl(U &&value) -> std::shared_ptr<snapshot<ValueType> const> {
    auto desired = std::make_shared<snapshot<ValueType>>(0, std::forward<U>(value));

    while (true) {
        auto expected = std::atomic_load(&mutable_current_);
        desired->version = expected->version + 1;

        auto const overwritten = std::atomic_compare_exchange_strong(&mutable_current_, &expected, desired);

        if (overwritten) return desired;
    }
}

template <class ValueType>
template <class Updater>
auto mvcc<ValueType>::update(Updater updater) -> std::shared_ptr<snapshot<ValueType> const> {
    while (true) {
        auto updated = this->try_update_impl(updater);
        if (updated != nullptr) return updated;

        std::this_thread::sleep_for(std::chrono::milliseconds(MVCC_CONTENSION_BACKOFF_SLEEP_MS));
    }
}

template <class ValueType>
template <class Updater>
auto mvcc<ValueType>::try_update_impl(Updater &updater) -> std::shared_ptr<snapshot<ValueType> const> {
    auto expected = std::atomic_load(&mutable_current_);
    auto const const_expected_version = expected->version;
    auto const &const_expected_value = expected->value;

    auto desired = std::make_shared<snapshot<ValueType>>(const_expected_version + 1,
                                                         updater(const_expected_version, const_expected_value));

    auto const updated = std::atomic_compare_exchange_strong(&mutable_current_, &expected, desired);

    if (updated) return desired;
    return nullptr;
}

template <class ValueType>
template <class Updater, class Clock, class Duration>
auto mvcc<ValueType>::try_update_until_impl(Updater &updater,
                                            std::chrono::time_point<Clock, Duration> const &timeout_time)
    -> std::shared_ptr<snapshot<ValueType> const> {
    while (true) {
        auto updated = this->try_update_impl(updater);

        if (updated != nullptr) return updated;

        if (std::chrono::high_resolution_clock::now() > timeout_time) return nullptr;

        std::this_thread::sleep_for(std::chrono::milliseconds(MVCC_CONTENSION_BACKOFF_SLEEP_MS));
    }
}
}

#endif
