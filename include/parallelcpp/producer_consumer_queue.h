#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace parallelcpp {

// A bounded-free, thread-safe FIFO queue for the classic producer/consumer
// pattern. Producers Push(); consumers either poll non-blockingly via
// TryPop() or block until an item is available (or the queue is marked
// finished) via WaitAndPop().
template <typename T>
class ProducerConsumerQueue {
public:
    void Push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        condition_.notify_one();
    }

    bool TryPop(T& out) {
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
    std::optional<T> WaitAndPop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty() || finished_; });
        if (queue_.empty()) return std::nullopt;  // finished_ and drained

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Signals that no more items will be pushed; wakes every thread
    // blocked in WaitAndPop() so they can observe an empty, finished queue
    // and exit instead of waiting forever.
    void SetFinished() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
        }
        condition_.notify_all();
    }

    bool Empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t Size() const {
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
