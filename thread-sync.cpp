#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <random>

/**
 * @brief Producer-Consumer pattern demonstration with thread synchronization
 * Shows usage of mutex, condition_variable, and thread-safe data structures
 */

class ProducerConsumerQueue {
private:
    std::queue<int> data_queue;
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    bool finished = false;

public:
    void push(int item) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        data_queue.push(item);
        condition.notify_one();
    }

    bool try_pop(int& item) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (data_queue.empty()) {
            return false;
        }
        item = data_queue.front();
        data_queue.pop();
        return true;
    }

    void wait_and_pop(int& item) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        while (data_queue.empty() && !finished) {
            condition.wait(lock);
        }
        if (!data_queue.empty()) {
            item = data_queue.front();
            data_queue.pop();
        }
    }

    void set_finished() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        finished = true;
        condition.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return data_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return data_queue.size();
    }
};

// Global shared data for demonstration
ProducerConsumerQueue shared_queue;
std::atomic<int> total_produced{0};
std::atomic<int> total_consumed{0};

void producer(int producer_id, int items_to_produce) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    std::uniform_int_distribution<> delay_dis(10, 50);

    for (int i = 0; i < items_to_produce; ++i) {
        int item = dis(gen);
        shared_queue.push(item);
        total_produced++;
        
        std::cout << "Producer " << producer_id 
                  << " produced: " << item << std::endl;
        
        // Simulate production time
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dis(gen)));
    }
    
    std::cout << "Producer " << producer_id << " finished" << std::endl;
}

void consumer(int consumer_id, int max_items) {
    std::uniform_int_distribution<> delay_dis(20, 80);
    std::random_device rd;
    std::mt19937 gen(rd());
    
    int consumed_count = 0;
    while (consumed_count < max_items) {
        int item;
        shared_queue.wait_and_pop(item);
        
        if (item > 0) {  // Valid item received
            total_consumed++;
            consumed_count++;
            
            std::cout << "Consumer " << consumer_id 
                      << " consumed: " << item << std::endl;
            
            // Simulate consumption time
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dis(gen)));
        }
    }
    
    std::cout << "Consumer " << consumer_id << " finished" << std::endl;
}

// Bank Account example demonstrating race condition prevention
class BankAccount {
private:
    mutable std::mutex account_mutex;
    double balance;

public:
    explicit BankAccount(double initial_balance) : balance(initial_balance) {}

    void deposit(double amount) {
        std::lock_guard<std::mutex> lock(account_mutex);
        balance += amount;
        std::cout << "Deposited: $" << amount 
                  << ", New balance: $" << balance << std::endl;
    }

    bool withdraw(double amount) {
        std::lock_guard<std::mutex> lock(account_mutex);
        if (balance >= amount) {
            balance -= amount;
            std::cout << "Withdrew: $" << amount 
                      << ", New balance: $" << balance << std::endl;
            return true;
        }
        std::cout << "Insufficient funds for withdrawal: $" << amount << std::endl;
        return false;
    }

    double get_balance() const {
        std::lock_guard<std::mutex> lock(account_mutex);
        return balance;
    }
};

void bank_operations(BankAccount& account, int operations_count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> amount_dis(10.0, 100.0);
    std::uniform_int_distribution<> op_dis(0, 1);

    for (int i = 0; i < operations_count; ++i) {
        double amount = amount_dis(gen);
        if (op_dis(gen) == 0) {
            account.deposit(amount);
        } else {
            account.withdraw(amount);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    std::cout << "=== Producer-Consumer Pattern Example ===" << std::endl;
    
    const int num_producers = 2;
    const int num_consumers = 3;
    const int items_per_producer = 5;
    
    std::vector<std::thread> threads;
    
    // Start producers
    for (int i = 0; i < num_producers; ++i) {
        threads.emplace_back(producer, i + 1, items_per_producer);
    }
    
    // Start consumers
    for (int i = 0; i < num_consumers; ++i) {
        threads.emplace_back(consumer, i + 1, 3);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < num_producers; ++i) {
        threads[i].join();
    }
    
    // Signal consumers that production is finished
    shared_queue.set_finished();
    
    // Wait for all consumers to finish
    for (int i = num_producers; i < threads.size(); ++i) {
        threads[i].join();
    }
    
    std::cout << "\nTotal produced: " << total_produced 
              << ", Total consumed: " << total_consumed << std::endl;
    
    std::cout << "\n=== Bank Account Thread Safety Example ===" << std::endl;
    
    BankAccount account(1000.0);
    std::vector<std::thread> bank_threads;
    
    // Create multiple threads performing bank operations
    for (int i = 0; i < 4; ++i) {
        bank_threads.emplace_back(bank_operations, std::ref(account), 5);
    }
    
    for (auto& t : bank_threads) {
        t.join();
    }
    
    std::cout << "\nFinal account balance: $" << account.get_balance() << std::endl;
    
    return 0;
}