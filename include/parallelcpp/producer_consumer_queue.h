#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace parallelcpp {

// A bounded-free, thread-safe FIFO queue for the classic producer/consumer
// pattern. Producers push(); consumers either poll non-blockingly via
// tryPop() or block until an item is available (or the queue is marked
// finished) via waitAndPop().
template <typename T>
class ProducerConsumerQueue {
public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        condition_.notify_one();
    }

    bool tryPop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Blocks until an item is available or the queue is marked finished.
    // Returns std::nullopt (rather than leaving an output parameter
    // unset) to make "nothing left, and nothing more is coming" an
    // explicit, unambiguous result instead of relying on the caller to
    // pre-initialize a sentinel value.
    std::optional<T> waitAndPop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty() || finished_; });
        if (queue_.empty()) return std::nullopt;  // finished_ and drained

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Signals that no more items will be pushed; wakes every thread
    // blocked in waitAndPop() so they can observe an empty, finished queue
    // and exit instead of waiting forever.
    void setFinished() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
        }
        condition_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool finished_ = false;
};

}  // namespace parallelcpp
