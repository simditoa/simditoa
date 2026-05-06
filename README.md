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

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and convert 1,000,000 `int64_t` values per iteration, repeated 10 times. A `bench_null` baseline measures the cost of just iterating the input vector, so per-conversion numbers can be read directly without subtracting loop overhead. The suite runs every implementation against two distributions and the three competitive implementations against a per-length sweep:

- **uniform** (`dist:0`): full `int64_t` range, uniform. Practically all values land in 18 or 19 digits.
- **log-uniform** (`dist:1`): uniform over bit-widths `[0, 63]` with random sign. Equal weight on each digit length, so short and long numbers contribute equally.
- **fixed length** (`dist:2`): positive values with exactly N digits, for N in {1, 4, 8, 12, 16, 19}.

The suite compares simditoa against `std::to_chars`, [jeaiii/itoa](https://github.com/jeaiii/itoa), [yy_itoa](https://github.com/ibireme/c_numconv_benchmark) (from ibireme's numconv benchmark suite), [rapidjson's branchlut writer](https://github.com/Tencent/rapidjson), and [fmtlib/fmt](https://github.com/fmtlib/fmt) (`fmt::format_int`).

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with the right flags to enable it:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSIMDITOA_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="-mavx512f -mavx512dq -mavx512bw -mavx512vl -mavx512ifma -mavx512vbmi"
```

### Results (AWS EC2, Intel Xeon Platinum 8488C @ 2.40 GHz, 8 vCPU)

#### Uniform distribution (`dist:0`, mostly 18–19 digit values)

| Implementation | ns/int | M ints/sec | GB/s |
|---|---:|---:|---:|
| `bench_null` (loop overhead) | 0.16 | 6,296 | 50.37 |
| `yy_itoa` | 7.21 | 138.73 | 2.69 |
| `simditoa::to_chars` (AVX-512 IFMA) | 9.32 | 107.27 | 2.08 |
| `jeaiii::to_text_from_integer` | 11.64 | 85.93 | 1.67 |
| `rapidjson` (branchlut) | 15.25 | 65.57 | 1.27 |
| `std::to_chars` | 20.12 | 49.69 | 0.96 |
| `fmt::format_int` | 34.83 | 28.71 | 0.56 |

On uniformly large values, `yy_itoa` is ~1.29x simditoa. simditoa is ~1.25x faster than jeaiii/itoa, ~1.64x faster than rapidjson's branchlut writer, ~2.16x faster than `std::to_chars`, and ~3.74x faster than `fmt::format_int`.

#### Log-uniform distribution (`dist:1`, all digit lengths equally weighted)

| Implementation | ns/int | M ints/sec | GB/s |
|---|---:|---:|---:|
| `bench_null` (loop overhead) | 0.16 | 6,322 | 50.57 |
| `simditoa::to_chars` (AVX-512 IFMA) | 11.50 | 86.92 | 0.87 |
| `yy_itoa` | 14.07 | 71.08 | 0.71 |
| `jeaiii::to_text_from_integer` | 16.57 | 60.36 | 0.61 |
| `rapidjson` (branchlut) | 20.77 | 48.15 | 0.48 |
| `std::to_chars` | 21.42 | 46.69 | 0.47 |
| `fmt::format_int` | 30.14 | 33.18 | 0.33 |

On a balanced mix of digit lengths, simditoa leads at 11.50 ns/int: ~1.22x yy_itoa and ~1.44x jeaiii. The crossover is explained by the per-length sweep below: the AVX-512 path has a near-constant per-call cost regardless of digit count, while scalar lookup methods scale with digit count.

#### Per-length sweep (`dist:2`)

ns/int by exact decimal length:

| digits | jeaiii | yy_itoa | simditoa |
|---:|---:|---:|---:|
| 1  | 0.93 | 1.76 | 3.00 |
| 4  | 1.60 | 2.23 | 3.00 |
| 8  | 2.35 | 3.11 | 3.00 |
| 12 | 3.87 | 4.49 | 3.77 |
| 16 | 4.42 | 5.57 | 3.75 |
| 19 | 6.56 | 6.74 | 5.06 |

simditoa is essentially flat at ~3 ns up to 8 digits (the SIMD pipeline cost dominates), then stays flat through 16 digits. jeaiii wins on values of 8 digits or fewer; from 12 digits onward simditoa is the fastest of the three. `yy_itoa` does not lead any length bucket on this hardware.

Run-to-run variation (`cv`) is at or under 1.66% across all measurements; most cases are under 0.2%, so means and medians can be compared directly.

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
