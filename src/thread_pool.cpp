#include "parallelcpp/thread_pool.h"

namespace parallelcpp {

ThreadPool::ThreadPool(std::size_t threadCount) {
    if (threadCount == 0) threadCount = 1;
    workers_.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    condition_.wait(lock, [this] { return tasks_.empty() && activeTasks_.load() == 0; });
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (stopping_.exchange(true)) return;  // already shut down
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();
}

std::size_t ThreadPool::queueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this] { return stopping_.load() || !tasks_.empty(); });

            if (stopping_.load() && tasks_.empty()) return;

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            task();
            activeTasks_--;
            totalCompleted_++;
            condition_.notify_all();  // wake any waitForAll() waiters
        }
    }
}

}  // namespace parallelcpp
