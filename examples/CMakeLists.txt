add_executable(thread_pool_demo thread_pool_demo.cpp)
target_link_libraries(thread_pool_demo PRIVATE parallelcpp)

add_executable(thread_sync_demo thread_sync_demo.cpp)
target_link_libraries(thread_sync_demo PRIVATE Threads::Threads)
target_include_directories(thread_sync_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)
