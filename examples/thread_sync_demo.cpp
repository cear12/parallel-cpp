// Demonstrates parallelcpp::ProducerConsumerQueue and parallelcpp::BankAccount:
// classic multi-producer/multi-consumer handoff, and race-free concurrent
// balance updates.
#include <atomic>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "parallelcpp/bank_account.h"
#include "parallelcpp/producer_consumer_queue.h"

using parallelcpp::BankAccount;
using parallelcpp::ProducerConsumerQueue;

namespace {

void ProducerConsumerDemo() {
  std::cout << "=== Producer/consumer queue ===\n";
  ProducerConsumerQueue<int> queue;
  std::atomic<int> produced{0}, consumed{0};

  constexpr int kProducers = 2, kItemsEach = 5, kConsumers = 3;
  std::vector<std::thread> threads;

  for (int p = 0; p < kProducers; ++p) {
    threads.emplace_back([&, p] {
      for (int i = 0; i < kItemsEach; ++i) {
        queue.Push(p * 100 + i);
        produced++;
      }
    });
  }
  for (int c = 0; c < kConsumers; ++c) {
    threads.emplace_back([&] {
      while (auto item = queue.WaitAndPop())
        consumed++;
    });
  }

  for (int p = 0; p < kProducers; ++p)
    threads[p].join();
  queue.SetFinished(); // wakes every consumer still blocked in WaitAndPop()
  for (int c = 0; c < kConsumers; ++c)
    threads[kProducers + c].join();

  std::cout << "produced=" << produced.load() << " consumed=" << consumed.load()
            << "\n";
}

void BankAccountDemo() {
  std::cout << "\n=== Concurrent bank account ===\n";
  BankAccount account(1000.0);
  std::vector<std::thread> threads;

  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&account, i] {
      std::mt19937 gen(static_cast<unsigned>(i));
      std::uniform_real_distribution<> amount_dist(10.0, 100.0);
      for (int op = 0; op < 5; ++op) {
        double amount = amount_dist(gen);
        if (op % 2 == 0) {
          account.Deposit(amount);
        } else {
          account.Withdraw(amount);
        }
      }
    });
  }
  for (auto &t : threads)
    t.join();

  std::cout << "Final balance: $" << account.Balance() << "\n";
}

} // namespace

int main() {
  ProducerConsumerDemo();
  BankAccountDemo();
}
