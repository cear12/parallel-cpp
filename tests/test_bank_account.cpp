#include <thread>
#include <vector>

#include "catch.hpp"
#include "parallelcpp/bank_account.h"

using parallelcpp::BankAccount;

TEST_CASE("deposit increases the balance", "[bank_account]") {
    BankAccount account(100.0);
    account.Deposit(50.0);
    REQUIRE(account.Balance() == Approx(150.0));
}

TEST_CASE("withdraw succeeds and decreases the balance when funds are sufficient", "[bank_account]") {
    BankAccount account(100.0);
    REQUIRE(account.Withdraw(30.0));
    REQUIRE(account.Balance() == Approx(70.0));
}

TEST_CASE("withdraw fails and leaves the balance unchanged on insufficient funds", "[bank_account]") {
    BankAccount account(50.0);
    REQUIRE_FALSE(account.Withdraw(100.0));
    REQUIRE(account.Balance() == Approx(50.0));
}

TEST_CASE("concurrent deposits from many threads never lose an update", "[bank_account]") {
    BankAccount account(0.0);
    constexpr int kThreads = 8;
    constexpr int kDepositsPerThread = 500;

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&account] {
            for (int j = 0; j < kDepositsPerThread; ++j) account.Deposit(1.0);
        });
    }
    for (auto& t : threads) t.join();

    // Without the mutex in BankAccount, this is exactly the read-modify-write
    // race that would make the total come out less than expected.
    REQUIRE(account.Balance() == Approx(kThreads * kDepositsPerThread));
}
