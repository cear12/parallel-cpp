#include "parallelcpp/thread_pool.h"

namespace parallelcpp {

ThreadPool::ThreadPool(std::size_t thread_count) {
    if (thread_count == 0) thread_count = 1;
    workers_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this] { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool() { Shutdown(); }

void ThreadPool::WaitForAll() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    condition_.wait(lock, [this] { return tasks_.empty() && active_tasks_.load() == 0; });
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stopping_.exchange(true)) return;  // already shut down
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();
}

std::size_t ThreadPool::QueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stopping_.load() || !tasks_.empty(); });

            if (stopping_.load() && tasks_.empty()) return;

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            task();
            active_tasks_--;
            total_completed_++;
            condition_.notify_all();  // wake any WaitForAll() waiters
        }
    }
}

}  // namespace parallelcpp
