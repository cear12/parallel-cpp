add_library(catch2_main STATIC test_main.cpp)
target_include_directories(catch2_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/catch2)

add_executable(parallelcpp_tests
    test_thread_pool.cpp
    test_producer_consumer_queue.cpp
    test_bank_account.cpp
)
target_link_libraries(parallelcpp_tests PRIVATE parallelcpp catch2_main)
target_include_directories(parallelcpp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/catch2)

include(CTest)
add_test(NAME parallelcpp_tests COMMAND parallelcpp_tests)
