enable_language(C)

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

FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
    SOURCE_SUBDIR _skip_rapidjson_cmake
)
FetchContent_MakeAvailable(rapidjson)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)

FetchContent_Declare(
    c_numconv_benchmark
    GIT_REPOSITORY https://github.com/ibireme/c_numconv_benchmark.git
    GIT_TAG master
    SOURCE_SUBDIR _skip_numconv_cmake
)
FetchContent_MakeAvailable(c_numconv_benchmark)

add_library(yy_itoa STATIC ${c_numconv_benchmark_SOURCE_DIR}/src/itoa/itoa_yy.c)
set_target_properties(yy_itoa PROPERTIES C_STANDARD 99 POSITION_INDEPENDENT_CODE ON)

add_executable(simditoa_benchmark simditoa_benchmark.cpp)
target_link_libraries(simditoa_benchmark PRIVATE simditoa benchmark::benchmark_main jeaiii_itoa fmt::fmt rapidjson yy_itoa)
