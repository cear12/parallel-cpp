#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace parallelcpp {

// A fixed-size worker-thread pool: enqueue() accepts any callable + its
// arguments and returns a std::future for the result, same shape as
// std::async but backed by a bounded, reused set of threads instead of
// spawning a new OS thread per call.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stopping_) {
                throw std::runtime_error("ThreadPool::enqueue called after shutdown()");
            }
            tasks_.emplace([task] { (*task)(); });
            activeTasks_++;
        }
        condition_.notify_one();
        return result;
    }

    // Blocks until every enqueued task (including ones still running) has
    // completed. Safe to call from any thread other than a worker itself.
    void waitForAll();

    void shutdown();

    std::size_t queueSize() const;
    std::size_t workerCount() const { return workers_.size(); }
    int activeTaskCount() const { return activeTasks_.load(); }
    int totalCompletedCount() const { return totalCompleted_.load(); }

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopping_{false};
    std::atomic<int> activeTasks_{0};
    std::atomic<int> totalCompleted_{0};
};

}  // namespace parallelcpp
