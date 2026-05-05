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

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and convert 1,000,000 random `int64_t` values, repeated 10 times. A `bench_null` baseline measures the cost of just iterating the input vector, so per-conversion numbers can be read directly without subtracting loop overhead. The suite compares simditoa against `std::to_chars` and [jeaiii/itoa](https://github.com/jeaiii/itoa).

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with the right flags to enable it:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIMDITOA_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="-mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512ifma -mavx512vbmi"
```

### Results (AWS EC2, Intel Xeon Platinum 8375C @ 2.90 GHz, 2 vCPU)

| Implementation | ns/int | M ints/sec | GB/s |
|---|---:|---:|---:|
| `bench_null` (loop overhead) | 0.14 | 6,916 | 55.32 |
| `std::to_chars` | 21.67 | 46.2 | 0.89 |
| `jeaiii::to_text_from_integer` | 12.38 | 80.8 | 1.57 |
| `simditoa::to_chars` (AVX-512 IFMA) | 10.02 | 99.8 | 1.93 |

Speedup vs `std::to_chars`: **2.16x**. simditoa is also ~1.24x faster than jeaiii/itoa on the same hardware.

```text
----------------------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------------------
bench_null/repeats:10_mean                    144639 ns       144604 ns           10 bytes/s=55.3242G/s ints/s=6.91553G/s ns/int=144.604ps
bench_null/repeats:10_median                  144508 ns       144455 ns           10 bytes/s=55.3806G/s ints/s=6.92258G/s ns/int=144.455ps
bench_null/repeats:10_cv                        0.34 %          0.33 %            10 bytes/s=0.33%     ints/s=0.33%      ns/int=0.33%
bench_std_to_chars/repeats:10_mean          21670905 ns     21665266 ns           10 bytes/s=894.46M/s  ints/s=46.1571M/s ns/int=21.6653ns
bench_std_to_chars/repeats:10_median        21668440 ns     21663994 ns           10 bytes/s=894.508M/s ints/s=46.1595M/s ns/int=21.664ns
bench_std_to_chars/repeats:10_cv                0.24 %          0.24 %            10 bytes/s=0.24%     ints/s=0.24%      ns/int=0.24%
bench_jeaiii_to_text/repeats:10_mean        12376965 ns     12375338 ns           10 bytes/s=1.56591G/s ints/s=80.806M/s  ns/int=12.3753ns
bench_jeaiii_to_text/repeats:10_median      12374998 ns     12373797 ns           10 bytes/s=1.5661G/s  ints/s=80.8159M/s ns/int=12.3738ns
bench_jeaiii_to_text/repeats:10_cv              0.14 %          0.14 %            10 bytes/s=0.14%     ints/s=0.14%      ns/int=0.14%
bench_simditoa_to_chars/repeats:10_mean     10020806 ns     10020124 ns           10 bytes/s=1.93397G/s ints/s=99.7992M/s ns/int=10.0201ns
bench_simditoa_to_chars/repeats:10_median   10021858 ns     10021265 ns           10 bytes/s=1.93375G/s ints/s=99.7878M/s ns/int=10.0213ns
bench_simditoa_to_chars/repeats:10_cv           0.05 %          0.05 %            10 bytes/s=0.05%     ints/s=0.05%      ns/int=0.05%
```

Run-to-run variation is under 0.4% on every case (`_cv` column), so the means and medians can be compared directly.

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
