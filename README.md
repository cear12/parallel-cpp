# parallel-cpp

Small, dependency-free C++20 building blocks for concurrent programming:
a thread pool with futures, a bounded producer/consumer queue, and a
thread-safe bank-account example used to demonstrate a real race condition
and its fix. Everything builds with only the standard library (`<thread>`,
`<mutex>`, `<condition_variable>`, `<future>`) — no third-party dependencies
are required to build the library or the examples.

## Modules

| Header | Description |
|---|---|
| `parallelcpp/thread_pool.h` | `ThreadPool` — fixed-size worker pool; `Enqueue()` returns a `std::future` for the task's result (or exception). Supports `WaitForAll()` and graceful `Shutdown()`. |
| `parallelcpp/producer_consumer_queue.h` | `ProducerConsumerQueue<T>` — header-only thread-safe queue. `WaitAndPop()` returns `std::optional<T>`, so a shutdown drain (queue empty, `SetFinished()` called) is expressed as `std::nullopt` instead of an uninitialized read. |
| `parallelcpp/bank_account.h` | `BankAccount` — minimal mutex-protected account (`Deposit`/`Withdraw`/`Balance`) used as a concurrency stress-test subject. |

## Bugs found and fixed while rewriting this repo

The original `thread-sync.cpp` had a real correctness bug: `wait_and_pop(int&
item)` returned without writing to `item` when the queue was empty and the
producer was finished, but the caller read `item` anyway to decide whether it
was valid. `ProducerConsumerQueue<T>::WaitAndPop()` fixes this by returning
`std::optional<T>` — there is no way to read a value that was never produced.
`tests/test_producer_consumer_queue.cpp` regression-tests this exact
shutdown-while-empty path with a real spawned `std::thread`.

The original `thread-pool.cpp` used the deprecated (removed-in-C++20)
`std::result_of`; `ThreadPool::Enqueue` now uses `std::invoke_result_t`.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake options (all default `ON`):

- `PARALLELCPP_BUILD_TESTS` — build the Catch2 unit test suite
- `PARALLELCPP_BUILD_EXAMPLES` — build `examples/`

## Building in Visual Studio

With three executable targets (`thread_pool_demo`, `thread_sync_demo`,
`parallelcpp_tests`) and no CMakePresets.json, Visual Studio's Open
Folder / CMake integration has no default startup item configured.
Pressing **Debug/Run** (not Build) then pops a blocking "Select Startup
Item" dialog -- easy to mistake for the project failing to build, even
though **Build > Build All** (Ctrl+Shift+B) succeeds regardless of
what's selected there. `CMakePresets.json` sets
`CMAKE_VS_STARTUP_PROJECT` to `thread_pool_demo` so Debug/Run works
immediately too; pick a different target from the dropdown next to the
Run button to debug either of the other two.

## Examples

- `examples/thread_pool_demo` — basic task submission, a mixed CPU/IO
  workload, batch processing of a vector of inputs, and exception
  propagation through `std::future::get()`.
- `examples/thread_sync_demo` — a producer/consumer pipeline over
  `ProducerConsumerQueue<int>`, plus a multi-threaded `BankAccount` demo
  showing correct behavior under concurrent deposits.

## Tests

Catch2 (vendored under `tests/third_party/catch2/`, single-header, v2,
BSL-1.0 license). 14 test cases covering: thread pool result/exception
propagation and actual parallel execution (verified via an atomic
concurrent-task counter), producer/consumer FIFO ordering and shutdown
semantics, and a bank-account stress test (8 threads x 500 deposits each,
asserting the final balance is exact) that would fail under the original
unsynchronized implementation.

## Status

Fully self-contained — builds and passes all tests with no system
dependencies beyond a C++20 compiler and threading support. CI builds and
tests on Linux, Windows, and macOS.
