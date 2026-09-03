#include <thread>

#include "catch.hpp"
#include "parallelcpp/producer_consumer_queue.h"

using parallelcpp::ProducerConsumerQueue;

TEST_CASE(
    "TryPop returns false on an empty queue and true once something is pushed",
    "[producer_consumer_queue]") {
  ProducerConsumerQueue<int> queue;
  int value = 0;
  REQUIRE_FALSE(queue.TryPop(value));

  queue.Push(42);
  REQUIRE(queue.TryPop(value));
  REQUIRE(value == 42);
  REQUIRE(queue.Empty());
}

TEST_CASE("WaitAndPop returns items in FIFO order",
          "[producer_consumer_queue]") {
  ProducerConsumerQueue<int> queue;
  queue.Push(1);
  queue.Push(2);
  queue.Push(3);

  REQUIRE(queue.WaitAndPop() == 1);
  REQUIRE(queue.WaitAndPop() == 2);
  REQUIRE(queue.WaitAndPop() == 3);
}

TEST_CASE("WaitAndPop unblocks with nullopt once SetFinished is called on an "
          "empty queue",
          "[producer_consumer_queue]") {
  ProducerConsumerQueue<int> queue;
  std::thread consumer([&] {
    auto result = queue.WaitAndPop();
    REQUIRE_FALSE(result.has_value());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  queue.SetFinished();
  consumer.join();
}

TEST_CASE("WaitAndPop delivers an already-queued item even after SetFinished",
          "[producer_consumer_queue]") {
  ProducerConsumerQueue<int> queue;
  queue.Push(99);
  queue.SetFinished();

  // Draining what's already there takes priority over "finished".
  REQUIRE(queue.WaitAndPop() == 99);
  REQUIRE_FALSE(queue.WaitAndPop().has_value());
}

TEST_CASE("Size() tracks pushes and pops accurately",
          "[producer_consumer_queue]") {
  ProducerConsumerQueue<int> queue;
  REQUIRE(queue.Size() == 0);
  queue.Push(1);
  queue.Push(2);
  REQUIRE(queue.Size() == 2);

  int out;
  queue.TryPop(out);
  REQUIRE(queue.Size() == 1);
}
