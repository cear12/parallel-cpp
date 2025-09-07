#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <chrono>
#include <random>

/**
 * @brief Advanced Thread Pool implementation with task queue,
 * work stealing, and performance monitoring
 */

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> active_tasks{0};
    std::atomic<int> total_tasks_completed{0};

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this, i] {
                worker_loop(i);
            });
        }
        std::cout << "Thread pool created with " << num_threads << " workers" << std::endl;
    }

    ~ThreadPool() {
        shutdown();
    }

    // Submit a task and get a future for the result
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop_flag) {
                throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
            }
            
            tasks.emplace([task] { (*task)(); });
            active_tasks++;
        }
        
        condition.notify_one();
        return result;
    }

    // Wait for all tasks to complete
    void wait_for_all() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        condition.wait(lock, [this] { 
            return tasks.empty() && active_tasks == 0; 
        });
    }

    // Get thread pool statistics
    void print_stats() const {
        std::cout << "Thread Pool Stats:" << std::endl;
        std::cout << "  Workers: " << workers.size() << std::endl;
        std::cout << "  Active tasks: " << active_tasks.load() << std::endl;
        std::cout << "  Queued tasks: " << get_queue_size() << std::endl;
        std::cout << "  Total completed: " << total_tasks_completed.load() << std::endl;
    }

    size_t get_queue_size() const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return tasks.size();
    }

    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        
        condition.notify_all();
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers.clear();
    }

private:
    void worker_loop(size_t worker_id) {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] { 
                    return stop_flag || !tasks.empty(); 
                });
                
                if (stop_flag && tasks.empty()) {
                    return;
                }
                
                if (!tasks.empty()) {
                    task = std::move(tasks.front());
                    tasks.pop();
                }
            }
            
            if (task) {
                task();
                active_tasks--;
                total_tasks_completed++;
                condition.notify_all(); // Notify waiting threads
            }
        }
    }
};

// Example tasks for demonstration
int cpu_intensive_task(int n) {
    // Simulate CPU-intensive work
    int result = 0;
    for (int i = 0; i < n * 10000; ++i) {
        result += std::sin(i) * std::cos(i);
    }
    
    std::cout << "CPU task completed with n=" << n 
              << " on thread " << std::this_thread::get_id() << std::endl;
    return result;
}

std::string io_simulation_task(const std::string& filename, int delay_ms) {
    // Simulate I/O operation
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    
    std::string result = "Processed file: " + filename;
    std::cout << result << " (delay: " << delay_ms << "ms) on thread " 
              << std::this_thread::get_id() << std::endl;
    return result;
}

void matrix_task(int size, int task_id) {
    // Simple matrix operation
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size, 1));
    
    long long sum = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            sum += matrix[i][j] * (i + j);
        }
    }
    
    std::cout << "Matrix task " << task_id << " (size " << size 
              << ") completed with sum=" << sum 
              << " on thread " << std::this_thread::get_id() << std::endl;
}

void demonstrate_basic_usage() {
    std::cout << "\n=== Basic Thread Pool Usage ===" << std::endl;
    
    ThreadPool pool(4);
    std::vector<std::future<int>> results;
    
    // Submit CPU-intensive tasks
    for (int i = 1; i <= 6; ++i) {
        results.push_back(pool.enqueue(cpu_intensive_task, i * 100));
    }
    
    std::cout << "Submitted 6 CPU-intensive tasks" << std::endl;
    pool.print_stats();
    
    // Collect results
    for (auto& result : results) {
        std::cout << "Task result: " << result.get() << std::endl;
    }
    
    pool.wait_for_all();
    std::cout << "All CPU tasks completed!" << std::endl;
    pool.print_stats();
}

void demonstrate_mixed_workload() {
    std::cout << "\n=== Mixed Workload Example ===" << std::endl;
    
    ThreadPool pool(6);
    std::vector<std::future<std::string>> io_futures;
    
    // Submit mixed I/O and CPU tasks
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dis(50, 300);
    
    // I/O simulation tasks
    for (int i = 0; i < 8; ++i) {
        std::string filename = "file_" + std::to_string(i) + ".txt";
        int delay = delay_dis(gen);
        io_futures.push_back(
            pool.enqueue(io_simulation_task, filename, delay)
        );
    }
    
    // CPU tasks (no return value needed)
    for (int i = 0; i < 5; ++i) {
        pool.enqueue(matrix_task, 200 + i * 50, i);
    }
    
    std::cout << "Submitted mixed workload (8 I/O + 5 matrix tasks)" << std::endl;
    pool.print_stats();
    
    // Process I/O results as they complete
    std::cout << "\nI/O Task Results:" << std::endl;
    for (auto& future : io_futures) {
        std::cout << "  " << future.get() << std::endl;
    }
    
    pool.wait_for_all();
    std::cout << "\nAll mixed workload completed!" << std::endl;
    pool.print_stats();
}

void demonstrate_batch_processing() {
    std::cout << "\n=== Batch Processing Example ===" << std::endl;
    
    ThreadPool pool(std::thread::hardware_concurrency());
    
    auto batch_task = [](int batch_id, const std::vector<int>& data) {
        long long sum = 0;
        for (int value : data) {
            sum += value * value;  // Square each value
        }
        
        std::cout << "Batch " << batch_id << " processed " << data.size() 
                  << " items, sum of squares: " << sum 
                  << " on thread " << std::this_thread::get_id() << std::endl;
        return sum;
    };
    
    // Create batches of data
    const int BATCH_SIZE = 10000;
    const int NUM_BATCHES = 8;
    
    std::vector<std::future<long long>> batch_results;
    
    for (int batch = 0; batch < NUM_BATCHES; ++batch) {
        std::vector<int> batch_data(BATCH_SIZE);
        std::iota(batch_data.begin(), batch_data.end(), batch * BATCH_SIZE);
        
        batch_results.push_back(
            pool.enqueue(batch_task, batch, batch_data)
        );
    }
    
    std::cout << "Submitted " << NUM_BATCHES << " batch processing tasks" << std::endl;
    
    // Collect and sum all batch results
    long long total_sum = 0;
    for (auto& result : batch_results) {
        total_sum += result.get();
    }
    
    std::cout << "Total sum of squares across all batches: " << total_sum << std::endl;
    pool.print_stats();
}

void demonstrate_exception_handling() {
    std::cout << "\n=== Exception Handling Example ===" << std::endl;
    
    ThreadPool pool(3);
    
    auto risky_task = [](int task_id, bool should_throw) -> int {
        if (should_throw) {
            throw std::runtime_error("Task " + std::to_string(task_id) + " failed!");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Task " << task_id << " completed successfully" << std::endl;
        return task_id * 10;
    };
    
    std::vector<std::future<int>> futures;
    
    // Submit mix of successful and failing tasks
    for (int i = 0; i < 6; ++i) {
        bool will_throw = (i % 3 == 0);  // Every third task will throw
        futures.push_back(pool.enqueue(risky_task, i, will_throw));
    }
    
    // Handle results and exceptions
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            int result = futures[i].get();
            std::cout << "Task " << i << " result: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Task " << i << " exception: " << e.what() << std::endl;
        }
    }
    
    pool.print_stats();
}

int main() {
    std::cout << "Advanced Thread Pool Examples" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    
    demonstrate_basic_usage();
    demonstrate_mixed_workload();
    demonstrate_batch_processing();
    demonstrate_exception_handling();
    
    std::cout << "\n=== All thread pool examples completed! ===" << std::endl;
    return 0;
}