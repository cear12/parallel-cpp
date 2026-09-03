#include <thread>

#include "catch.hpp"
#include "parallelcpp/producer_consumer_queue.h"

using parallelcpp::ProducerConsumerQueue;

TEST_CASE("tryPop returns false on an empty queue and true once something is pushed", "[producer_consumer_queue]") {
    ProducerConsumerQueue<int> queue;
    int value = 0;
    REQUIRE_FALSE(queue.tryPop(value));

    queue.push(42);
    REQUIRE(queue.tryPop(value));
    REQUIRE(value == 42);
    REQUIRE(queue.empty());
}

TEST_CASE("waitAndPop returns items in FIFO order", "[producer_consumer_queue]") {
    ProducerConsumerQueue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    REQUIRE(queue.waitAndPop() == 1);
    REQUIRE(queue.waitAndPop() == 2);
    REQUIRE(queue.waitAndPop() == 3);
}

TEST_CASE("waitAndPop unblocks with nullopt once setFinished is called on an empty queue",
          "[producer_consumer_queue]") {
    ProducerConsumerQueue<int> queue;
    std::thread consumer([&] {
        auto result = queue.waitAndPop();
        REQUIRE_FALSE(result.has_value());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    queue.setFinished();
    consumer.join();
}

TEST_CASE("waitAndPop delivers an already-queued item even after setFinished", "[producer_consumer_queue]") {
    ProducerConsumerQueue<int> queue;
    queue.push(99);
    queue.setFinished();

    // Draining what's already there takes priority over "finished".
    REQUIRE(queue.waitAndPop() == 99);
    REQUIRE_FALSE(queue.waitAndPop().has_value());
}

TEST_CASE("size() tracks pushes and pops accurately", "[producer_consumer_queue]") {
    ProducerConsumerQueue<int> queue;
    REQUIRE(queue.size() == 0);
    queue.push(1);
    queue.push(2);
    REQUIRE(queue.size() == 2);

    int out;
    queue.tryPop(out);
    REQUIRE(queue.size() == 1);
}
