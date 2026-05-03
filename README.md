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

Example benchmark run:

| Implementation | Best time | Throughput |
|---|---:|---:|
| `std::to_chars` | 36.35 ns/int | 27.51 M ints/sec |
| `simditoa::to_chars` | 15.82 ns/int | 63.22 M ints/sec |

Speedup: **2.30x**.

```text
std::to_chars (baseline)
  iter  1: 45.56 ns/int  (21.95 M ints/sec)
  iter  2: 40.89 ns/int  (24.46 M ints/sec)
  iter  3: 36.60 ns/int  (27.32 M ints/sec)
  iter  4: 36.49 ns/int  (27.41 M ints/sec)
  iter  5: 36.59 ns/int  (27.33 M ints/sec)
  iter  6: 36.61 ns/int  (27.31 M ints/sec)
  iter  7: 36.35 ns/int  (27.51 M ints/sec)
  iter  8: 37.57 ns/int  (26.62 M ints/sec)
  iter  9: 36.54 ns/int  (27.37 M ints/sec)
  iter 10: 37.04 ns/int  (27.00 M ints/sec)

  best: 36.35 ns/int  (27.51 M ints/sec)
  total chars (last run): 19378610

simditoa::to_chars
  iter  1: 16.08 ns/int  (62.17 M ints/sec)
  iter  2: 16.16 ns/int  (61.88 M ints/sec)
  iter  3: 15.86 ns/int  (63.05 M ints/sec)
  iter  4: 15.89 ns/int  (62.92 M ints/sec)
  iter  5: 15.87 ns/int  (63.02 M ints/sec)
  iter  6: 15.98 ns/int  (62.59 M ints/sec)
  iter  7: 15.96 ns/int  (62.66 M ints/sec)
  iter  8: 15.82 ns/int  (63.22 M ints/sec)
  iter  9: 16.09 ns/int  (62.17 M ints/sec)
  iter 10: 15.95 ns/int  (62.69 M ints/sec)

  best: 15.82 ns/int  (63.22 M ints/sec)
  total chars (last run): 19378610

--- Summary ---
  std::to_chars:    36.35 ns/int
  simditoa:         15.82 ns/int
  speedup:          2.30x
```

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
FetchContent_Declare(simditoa GIT_REPOSITORY https://github.com/user/simditoa.git GIT_TAG main)
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
