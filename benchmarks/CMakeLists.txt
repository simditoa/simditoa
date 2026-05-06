include(FetchContent)

FetchContent_Declare(
    google_benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.5.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(google_benchmark)

FetchContent_Declare(
    jeaiii_itoa
    GIT_REPOSITORY https://github.com/jeaiii/itoa.git
    GIT_TAG main
)
FetchContent_MakeAvailable(jeaiii_itoa)

add_library(jeaiii_itoa INTERFACE)
target_include_directories(jeaiii_itoa INTERFACE ${jeaiii_itoa_SOURCE_DIR}/include/itoa)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.1.4
)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt)

add_executable(simditoa_benchmark simditoa_benchmark.cpp)
target_link_libraries(simditoa_benchmark PRIVATE simditoa benchmark::benchmark_main jeaiii_itoa fmt::fmt)
