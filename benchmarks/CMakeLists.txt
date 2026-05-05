include(FetchContent)

FetchContent_Declare(
    google_benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.5.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(google_benchmark)

add_executable(simditoa_benchmark simditoa_benchmark.cpp)
target_link_libraries(simditoa_benchmark PRIVATE simditoa benchmark::benchmark_main)
