#include <atomic>
#include <chrono>

#include "catch.hpp"
#include "parallelcpp/thread_pool.h"

using parallelcpp::ThreadPool;

TEST_CASE("ThreadPool runs a submitted task and returns its result", "[thread_pool]") {
    ThreadPool pool(2);
    auto future = pool.Enqueue([](int a, int b) { return a + b; }, 2, 3);
    REQUIRE(future.get() == 5);
}

TEST_CASE("ThreadPool actually parallelizes across multiple workers", "[thread_pool]") {
    ThreadPool pool(4);
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_observed{0};

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.push_back(pool.Enqueue([&] {
            int now = ++concurrent_count;
            int prev_max = max_observed.load();
            while (now > prev_max && !max_observed.compare_exchange_weak(prev_max, now)) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            --concurrent_count;
        }));
    }
    for (auto& f : futures) f.get();

    // With 4 workers and 4 tasks that each sleep, more than one must have
    // been in flight at once -- this would be 1 on a serial "pool".
    REQUIRE(max_observed.load() > 1);
}

TEST_CASE("ThreadPool propagates exceptions through the returned future", "[thread_pool]") {
    ThreadPool pool(2);
    auto future = pool.Enqueue([]() -> int { throw std::runtime_error("boom"); });
    REQUIRE_THROWS_WITH(future.get(), "boom");
}

TEST_CASE("ThreadPool::WaitForAll blocks until every task has completed", "[thread_pool]") {
    ThreadPool pool(3);
    std::atomic<int> completed{0};
    for (int i = 0; i < 10; ++i) {
        pool.Enqueue([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            completed++;
        });
    }
    pool.WaitForAll();
    REQUIRE(completed.load() == 10);
    REQUIRE(pool.TotalCompletedCount() == 10);
}

TEST_CASE("ThreadPool::Enqueue throws after Shutdown", "[thread_pool]") {
    ThreadPool pool(1);
    pool.Shutdown();
    REQUIRE_THROWS_AS(pool.Enqueue([] { return 1; }), std::runtime_error);
}
