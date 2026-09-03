#pragma once

#include <mutex>

namespace parallelcpp {

// A minimal thread-safe account balance: every operation takes the same
// mutex, so concurrent Deposit()/Withdraw() calls from multiple threads
// can't race and corrupt the balance (the classic motivating example for
// why shared mutable state needs a lock).
class BankAccount {
public:
    explicit BankAccount(double initial_balance) : balance_(initial_balance) {}

    void Deposit(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        balance_ += amount;
    }

    // Returns false (rather than throwing) on insufficient funds --
    // callers decide whether that's an error or an expected outcome.
    bool Withdraw(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (balance_ < amount) return false;
        balance_ -= amount;
        return true;
    }

    double Balance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return balance_;
    }

private:
    mutable std::mutex mutex_;
    double balance_;
};

}  // namespace parallelcpp
