# simditoa

SIMD-accelerated 64-bit integer-to-string conversion.

## Supported architectures

| Architecture | Instruction set | Notes |
|---|---|---|
| x86-64 | AVX-512 IFMA + VBMI | Intel Ice Lake+, AMD Zen 4+ |
| Any | Scalar fallback | Portable C++17 |

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with `-mavx512ifma -mavx512vbmi` (and the surrounding AVX-512 flags) to enable it.

## API

```cpp
#include "simditoa.h"

char buf[simditoa::MAX_DIGITS + 1];
size_t len = simditoa::to_chars(12345, buf);
buf[len] = '\0';
```

Both `int64_t` and `uint64_t` overloads are provided. The buffer must hold at least `simditoa::MAX_DIGITS + 1` (21) bytes.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

With tests:

```bash
cmake -B build -DSIMDITOA_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build
```

## Install

```bash
cmake --install build --prefix /usr/local
```

Use it from CMake:

```cmake
find_package(simditoa REQUIRED)
target_link_libraries(myapp PRIVATE simditoa::simditoa)
```

Or via `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(simditoa GIT_REPOSITORY https://github.com/simditoa/simditoa.git GIT_TAG main)
FetchContent_MakeAvailable(simditoa)
target_link_libraries(myapp PRIVATE simditoa::simditoa)
```

## Benchmarks

Benchmarks live in a dedicated repository: [simditoa/benchmarks](https://github.com/simditoa/benchmarks). It compares simditoa against `std::to_chars`, `jeaiii/itoa`, `yy_itoa`, `rapidjson's branchlut writer`, and `fmtlib`.

Latest run on a GCP `c3-standard-8` (Intel Xeon Platinum 8481C, AVX-512 IFMA + VBMI), 2026-05-07: simditoa wins the realistic-subset geomean at **194.8M ints/s**, with **269.7M ints/s** on `UNIFORM_POS` and **186.2M ints/s** at 19 fixed digits. See the benchmark repo's `RESULTS.md` for the full breakdown.

## Algorithm

The AVX-512 implementation is based on:

> Champagne Gareau & Lemire, "Converting an Integer to a Decimal String in Under Two Nanoseconds," arXiv:2604.26019, 2026.

It uses AVX-512 IFMA (`vpmadd52lo`/`vpmadd52hi`) with precomputed constants `c_k = ⌊2^52 / 10^k⌋` to extract all 8 decimal digits in parallel without division.

## License

Dual-licensed under [MIT](LICENSE-MIT) and [Apache 2.0](LICENSE-APACHE).
