// Demonstrates parallelcpp::ThreadPool: basic submit/collect, a mixed
// CPU + I/O-simulated workload, batch processing, and exception
// propagation through std::future.
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include "parallelcpp/thread_pool.h"

using parallelcpp::ThreadPool;

namespace {

void PrintStats(const ThreadPool& pool) {
    std::cout << "  workers=" << pool.WorkerCount() << " active=" << pool.ActiveTaskCount()
              << " queued=" << pool.QueueSize() << " completed=" << pool.TotalCompletedCount() << "\n";
}

int CpuIntensiveTask(int n) {
    double result = 0;
    for (int i = 0; i < n * 10000; ++i) result += std::sin(i) * std::cos(i);
    return static_cast<int>(result);
}

std::string IoSimulationTask(const std::string& filename, int delay_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    return "processed " + filename;
}

void BasicUsage() {
    std::cout << "\n=== Basic usage ===\n";
    ThreadPool pool(4);
    std::vector<std::future<int>> results;
    for (int i = 1; i <= 6; ++i) results.push_back(pool.Enqueue(CpuIntensiveTask, i * 100));

    std::cout << "Submitted 6 CPU-bound tasks\n";
    for (auto& r : results) std::cout << "  result=" << r.get() << "\n";
    pool.WaitForAll();
    PrintStats(pool);
}

void MixedWorkload() {
    std::cout << "\n=== Mixed CPU + I/O workload ===\n";
    ThreadPool pool(6);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dist(5, 20);

    std::vector<std::future<std::string>> io_results;
    for (int i = 0; i < 8; ++i) {
        io_results.push_back(pool.Enqueue(IoSimulationTask, "file_" + std::to_string(i) + ".txt", delay_dist(gen)));
    }
    for (int i = 0; i < 5; ++i) pool.Enqueue(CpuIntensiveTask, 50 + i * 10);

    for (auto& r : io_results) std::cout << "  " << r.get() << "\n";
    pool.WaitForAll();
    PrintStats(pool);
}

void BatchProcessing() {
    std::cout << "\n=== Batch processing ===\n";
    ThreadPool pool(std::thread::hardware_concurrency());

    auto batch_task = [](const std::vector<int>& data) {
        long long sum = 0;
        for (int v : data) sum += static_cast<long long>(v) * v;
        return sum;
    };

    constexpr int kBatchSize = 1000;
    constexpr int kNumBatches = 8;
    std::vector<std::future<long long>> results;
    for (int batch = 0; batch < kNumBatches; ++batch) {
        std::vector<int> data(kBatchSize);
        std::iota(data.begin(), data.end(), batch * kBatchSize);
        results.push_back(pool.Enqueue(batch_task, data));
    }

    long long total = 0;
    for (auto& r : results) total += r.get();
    std::cout << "Sum of squares across " << kNumBatches << " batches: " << total << "\n";
}

void ExceptionHandling() {
    std::cout << "\n=== Exception propagation through std::future ===\n";
    ThreadPool pool(3);

    auto risky_task = [](int id, bool should_throw) -> int {
        if (should_throw) throw std::runtime_error("task " + std::to_string(id) + " failed");
        return id * 10;
    };

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 6; ++i) futures.push_back(pool.Enqueue(risky_task, i, i % 3 == 0));

    for (std::size_t i = 0; i < futures.size(); ++i) {
        try {
            int result = futures[i].get();
            std::cout << "  task " << i << " result: " << result << "\n";
        } catch (const std::exception& e) {
            std::cout << "  task " << i << " threw: " << e.what() << "\n";
        }
    }
}

}  // namespace

int main() {
    std::cout << "hardware_concurrency = " << std::thread::hardware_concurrency() << "\n";
    BasicUsage();
    MixedWorkload();
    BatchProcessing();
    ExceptionHandling();
    std::cout << "\nAll thread pool demos completed.\n";
}
