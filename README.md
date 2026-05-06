# simditoa

SIMD-accelerated integer-to-string conversion.

Converts 64-bit integers to decimal strings using SIMD instructions where available, achieving sub-2-nanosecond conversion on modern hardware.

## Supported Architectures

| Architecture | Instruction Set | Notes |
|---|---|---|
| x86-64 | AVX-512 IFMA + VBMI | Intel Ice Lake+, AMD Zen 4+ |
| Any | Scalar fallback | Portable C++17 |

## API

```cpp
#include "simditoa.h"

char buf[simditoa::MAX_DIGITS + 1];
size_t len = simditoa::to_chars(12345, buf);
buf[len] = '\0';
```

Both `int64_t` and `uint64_t` overloads are provided. The buffer must hold at least `simditoa::MAX_DIGITS + 1` (21) bytes.

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### With tests and benchmarks

```bash
cmake -B build -DSIMDITOA_DEVELOPER_MODE=ON -DSIMDITOA_BUILD_BENCHMARKS=ON
cmake --build build
ctest --test-dir build
./build/benchmarks/simditoa_benchmark
```

## Benchmark Result

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and convert 1,000,000 random `int64_t` values, repeated 10 times. A `bench_null` baseline measures the cost of just iterating the input vector, so per-conversion numbers can be read directly without subtracting loop overhead. The suite compares simditoa against `std::to_chars`, [jeaiii/itoa](https://github.com/jeaiii/itoa), [yy_itoa](https://github.com/ibireme/c_numconv_benchmark) (from ibireme's numconv benchmark suite), [rapidjson's branchlut writer](https://github.com/Tencent/rapidjson), and [fmtlib/fmt](https://github.com/fmtlib/fmt) (`fmt::format_int`).

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with the right flags to enable it:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIMDITOA_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="-mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512ifma -mavx512vbmi"
```

### Results (AWS EC2, Intel Xeon Platinum 8488C @ 2.40 GHz, 8 vCPU)

| Implementation | ns/int | M ints/sec | GB/s |
|---|---:|---:|---:|
| `bench_null` (loop overhead) | 0.14 | 7,045 | 56.36 |
| `fmt::format_int` | 31.19 | 32.06 | 0.62 |
| `std::to_chars` | 18.00 | 55.55 | 1.08 |
| `rapidjson` (branchlut) | 13.64 | 73.34 | 1.42 |
| `jeaiii::to_text_from_integer` | 10.38 | 96.35 | 1.87 |
| `simditoa::to_chars` (AVX-512 IFMA) | 8.24 | 121.36 | 2.35 |
| `yy_itoa` | 6.41 | 156.01 | 3.02 |

Speedup vs `std::to_chars`: **2.18x**. simditoa is ~1.26x faster than jeaiii/itoa, ~1.66x faster than rapidjson's branchlut writer, and ~3.79x faster than `fmt::format_int`. `yy_itoa` is the fastest implementation in this set at ~1.29x simditoa, showing that a well-tuned scalar lookup approach still leads on this hardware. fmt's general-purpose formatting machinery makes its `format_int` shortcut the slowest of the implementations measured here, despite avoiding allocation.

```text
------------------------------------------------------------------------------------------------------
Benchmark                                            Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------------------
bench_null/repeats:10_mean                      141991 ns       141965 ns           10 bytes/s=56.3571G/s ints/s=7.04464G/s ns/int=141.965ps
bench_null/repeats:10_median                    141571 ns       141539 ns           10 bytes/s=56.5217G/s ints/s=7.06521G/s ns/int=141.539ps
bench_null/repeats:10_cv                          1.03 %          1.03 %            10 bytes/s=1.00%     ints/s=1.00%      ns/int=1.03%
bench_std_to_chars/repeats:10_mean            18003617 ns     18001726 ns           10 bytes/s=1.07655G/s ints/s=55.5534M/s ns/int=18.0017ns
bench_std_to_chars/repeats:10_median          17992497 ns     17990446 ns           10 bytes/s=1.07716G/s ints/s=55.5851M/s ns/int=17.9904ns
bench_std_to_chars/repeats:10_cv                  0.80 %          0.80 %            10 bytes/s=0.80%     ints/s=0.80%      ns/int=0.80%
bench_jeaiii_to_text/repeats:10_mean          10380266 ns     10379232 ns           10 bytes/s=1.86712G/s ints/s=96.3495M/s ns/int=10.3792ns
bench_jeaiii_to_text/repeats:10_median        10378474 ns     10377485 ns           10 bytes/s=1.86738G/s ints/s=96.3627M/s ns/int=10.3775ns
bench_jeaiii_to_text/repeats:10_cv                0.61 %          0.61 %            10 bytes/s=0.61%     ints/s=0.61%      ns/int=0.61%
bench_yy_itoa/repeats:10_mean                  6410323 ns      6409909 ns           10 bytes/s=3.02332G/s ints/s=156.013M/s ns/int=6.40991ns
bench_yy_itoa/repeats:10_median                6399560 ns      6399186 ns           10 bytes/s=3.02831G/s ints/s=156.271M/s ns/int=6.39919ns
bench_yy_itoa/repeats:10_cv                       0.58 %          0.58 %            10 bytes/s=0.58%     ints/s=0.58%      ns/int=0.58%
bench_rapidjson_branchlut/repeats:10_mean     13636372 ns     13635348 ns           10 bytes/s=1.42123G/s ints/s=73.3402M/s ns/int=13.6353ns
bench_rapidjson_branchlut/repeats:10_median   13629011 ns     13628124 ns           10 bytes/s=1.42196G/s ints/s=73.3779M/s ns/int=13.6281ns
bench_rapidjson_branchlut/repeats:10_cv           0.46 %          0.45 %            10 bytes/s=0.45%     ints/s=0.45%      ns/int=0.45%
bench_fmt_format_int/repeats:10_mean          31195365 ns     31193564 ns           10 bytes/s=621.243M/s ints/s=32.0582M/s ns/int=31.1936ns
bench_fmt_format_int/repeats:10_median        31207106 ns     31204937 ns           10 bytes/s=621.011M/s ints/s=32.0462M/s ns/int=31.2049ns
bench_fmt_format_int/repeats:10_cv                0.31 %          0.31 %            10 bytes/s=0.31%     ints/s=0.31%      ns/int=0.31%
bench_simditoa_to_chars/repeats:10_mean        8241230 ns      8240672 ns           10 bytes/s=2.35172G/s ints/s=121.357M/s ns/int=8.24067ns
bench_simditoa_to_chars/repeats:10_median      8237885 ns      8237178 ns           10 bytes/s=2.35258G/s ints/s=121.401M/s ns/int=8.23718ns
bench_simditoa_to_chars/repeats:10_cv             0.81 %          0.81 %            10 bytes/s=0.81%     ints/s=0.81%      ns/int=0.81%
```

Run-to-run variation is at or under 1.03% on every case. `bench_null` is the noisiest at 1.03%; every real conversion benchmark measures under 0.81%. Means and medians can be compared directly.

### Install

```bash
cmake --install build --prefix /usr/local
```

After installation, use in your CMake project:

```cmake
find_package(simditoa REQUIRED)
target_link_libraries(myapp PRIVATE simditoa::simditoa)
```

Or use directly with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(simditoa GIT_REPOSITORY https://github.com/simditoa/simditoa.git GIT_TAG main)
FetchContent_MakeAvailable(simditoa)
target_link_libraries(myapp PRIVATE simditoa::simditoa)
```

## Algorithm

The AVX-512 implementation is based on:

> Champagne Gareau & Lemire, "Converting an Integer to a Decimal String in Under Two Nanoseconds," arXiv:2604.26019, 2026.

The key insight uses AVX-512 IFMA instructions (`vpmadd52lo`/`vpmadd52hi`) with precomputed constants `c_k = ⌊2^52 / 10^k⌋` to extract all 8 decimal digits of a number in parallel without any division.

## Project Structure

```
include/simditoa.h              — public umbrella header
include/simditoa/               — internal public headers
src/simditoa.cpp                — unity build source
src/avx512.cpp                  — AVX-512 IFMA implementation
src/fallback.cpp                — portable scalar implementation
tests/                          — test suite
benchmarks/                     — performance benchmarks
cmake/                          — CMake helper modules
```

## License

Dual-licensed under [MIT](LICENSE-MIT) and [Apache 2.0](LICENSE-APACHE).
