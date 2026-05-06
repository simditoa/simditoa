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

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and convert 1,000,000 random `int64_t` values, repeated 10 times. A `bench_null` baseline measures the cost of just iterating the input vector, so per-conversion numbers can be read directly without subtracting loop overhead. The suite compares simditoa against `std::to_chars`, [jeaiii/itoa](https://github.com/jeaiii/itoa), and [fmtlib/fmt](https://github.com/fmtlib/fmt) (`fmt::format_int`).

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with the right flags to enable it:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIMDITOA_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="-mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512ifma -mavx512vbmi"
```

### Results (AWS EC2, Intel Xeon Platinum 8375C @ 2.90 GHz, 2 vCPU)

| Implementation | ns/int | M ints/sec | GB/s |
|---|---:|---:|---:|
| `bench_null` (loop overhead) | 0.14 | 6,919 | 55.35 |
| `fmt::format_int` | 34.58 | 28.9 | 0.56 |
| `std::to_chars` | 22.14 | 45.2 | 0.88 |
| `jeaiii::to_text_from_integer` | 12.21 | 81.9 | 1.59 |
| `simditoa::to_chars` (AVX-512 IFMA) | 9.99 | 100.1 | 1.94 |

Speedup vs `std::to_chars`: **2.22x**. simditoa is also ~1.22x faster than jeaiii/itoa and ~3.46x faster than `fmt::format_int` on the same hardware. fmt's general-purpose formatting machinery makes its `format_int` shortcut the slowest of the four implementations measured here, despite avoiding allocation.

```text
----------------------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------
bench_null/repeats:10_mean                    144552 ns       144528 ns           10 bytes/s=55.3526G/s ints/s=6.91907G/s ns/int=144.528ps
bench_null/repeats:10_median                  144512 ns       144488 ns           10 bytes/s=55.3677G/s ints/s=6.92097G/s ns/int=144.488ps
bench_null/repeats:10_cv                        0.11 %          0.12 %            10 bytes/s=0.12%     ints/s=0.12%      ns/int=0.12%
bench_std_to_chars/repeats:10_mean          22142972 ns     22137702 ns           10 bytes/s=876.07M/s  ints/s=45.2081M/s ns/int=22.1377ns
bench_std_to_chars/repeats:10_median        21826691 ns     21816894 ns           10 bytes/s=888.239M/s ints/s=45.836M/s  ns/int=21.8169ns
bench_std_to_chars/repeats:10_cv                3.05 %          3.05 %            10 bytes/s=2.92%     ints/s=2.92%      ns/int=3.05%
bench_jeaiii_to_text/repeats:10_mean        12213726 ns     12212966 ns           10 bytes/s=1.58673G/s ints/s=81.8803M/s ns/int=12.213ns
bench_jeaiii_to_text/repeats:10_median      12221402 ns     12220576 ns           10 bytes/s=1.58574G/s ints/s=81.8292M/s ns/int=12.2206ns
bench_jeaiii_to_text/repeats:10_cv              0.14 %          0.14 %            10 bytes/s=0.14%     ints/s=0.14%      ns/int=0.14%
bench_fmt_format_int/repeats:10_mean        34585620 ns     34583919 ns           10 bytes/s=560.336M/s ints/s=28.9152M/s ns/int=34.5839ns
bench_fmt_format_int/repeats:10_median      34578580 ns     34577292 ns           10 bytes/s=560.443M/s ints/s=28.9207M/s ns/int=34.5773ns
bench_fmt_format_int/repeats:10_cv              0.09 %          0.09 %            10 bytes/s=0.09%     ints/s=0.09%      ns/int=0.09%
bench_simditoa_to_chars/repeats:10_mean      9994703 ns      9994211 ns           10 bytes/s=1.93898G/s ints/s=100.058M/s ns/int=9.99421ns
bench_simditoa_to_chars/repeats:10_median    9994022 ns      9993360 ns           10 bytes/s=1.93915G/s ints/s=100.066M/s ns/int=9.99336ns
bench_simditoa_to_chars/repeats:10_cv           0.09 %          0.09 %            10 bytes/s=0.09%     ints/s=0.09%      ns/int=0.09%
```

Run-to-run variation is under 0.15% on every case except `std::to_chars` (3.05%), where the libstdc++ implementation shows higher noise. Means and medians can still be compared directly.

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
