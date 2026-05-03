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

> Champagne Gareau & Lemire, "Converting an Integer to a Decimal String in Under Two Nanoseconds," arXiv:2604.26019, 2025.

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

