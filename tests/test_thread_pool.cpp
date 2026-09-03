#include <atomic>
#include <chrono>

#include "catch.hpp"
#include "parallelcpp/thread_pool.h"

using parallelcpp::ThreadPool;

TEST_CASE("ThreadPool runs a submitted task and returns its result", "[thread_pool]") {
    ThreadPool pool(2);
    auto future = pool.enqueue([](int a, int b) { return a + b; }, 2, 3);
    REQUIRE(future.get() == 5);
}

TEST_CASE("ThreadPool actually parallelizes across multiple workers", "[thread_pool]") {
    ThreadPool pool(4);
    std::atomic<int> concurrentCount{0};
    std::atomic<int> maxObserved{0};

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.push_back(pool.enqueue([&] {
            int now = ++concurrentCount;
            int prevMax = maxObserved.load();
            while (now > prevMax && !maxObserved.compare_exchange_weak(prevMax, now)) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            --concurrentCount;
        }));
    }
    for (auto& f : futures) f.get();

    // With 4 workers and 4 tasks that each sleep, more than one must have
    // been in flight at once -- this would be 1 on a serial "pool".
    REQUIRE(maxObserved.load() > 1);
}

TEST_CASE("ThreadPool propagates exceptions through the returned future", "[thread_pool]") {
    ThreadPool pool(2);
    auto future = pool.enqueue([]() -> int { throw std::runtime_error("boom"); });
    REQUIRE_THROWS_WITH(future.get(), "boom");
}

TEST_CASE("ThreadPool::waitForAll blocks until every task has completed", "[thread_pool]") {
    ThreadPool pool(3);
    std::atomic<int> completed{0};
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            completed++;
        });
    }
    pool.waitForAll();
    REQUIRE(completed.load() == 10);
    REQUIRE(pool.totalCompletedCount() == 10);
}

TEST_CASE("ThreadPool::enqueue throws after shutdown", "[thread_pool]") {
    ThreadPool pool(1);
    pool.shutdown();
    REQUIRE_THROWS_AS(pool.enqueue([] { return 1; }), std::runtime_error);
}
