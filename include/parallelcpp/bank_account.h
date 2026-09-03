#pragma once

#include <mutex>

namespace parallelcpp {

// A minimal thread-safe account balance: every operation takes the same
// mutex, so concurrent deposit()/withdraw() calls from multiple threads
// can't race and corrupt the balance (the classic motivating example for
// why shared mutable state needs a lock).
class BankAccount {
public:
    explicit BankAccount(double initialBalance) : balance_(initialBalance) {}

    void deposit(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        balance_ += amount;
    }

    // Returns false (rather than throwing) on insufficient funds --
    // callers decide whether that's an error or an expected outcome.
    bool withdraw(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (balance_ < amount) return false;
        balance_ -= amount;
        return true;
    }

    double balance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return balance_;
    }

private:
    mutable std::mutex mutex_;
    double balance_;
};

}  // namespace parallelcpp
