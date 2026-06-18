# simditoa

SIMD-accelerated 64-bit integer-to-string conversion.

## Supported architectures

| Architecture | Instruction set | Notes |
|---|---|---|
| x86-64 | AVX-512 IFMA + VBMI | Intel Ice Lake+, AMD Zen 4+ |
| RISC-V (rv64) | Scalar fallback | Portable C++17; no RVV backend yet |
| Any | Scalar fallback | Portable C++17 |

The AVX-512 path is gated at compile time on `__AVX512IFMA__`. Build with `-mavx512ifma -mavx512vbmi` (and the surrounding AVX-512 flags) to enable it.
For `riscv64`, the current portability path is the scalar fallback only, without RVV intrinsics or runtime dispatch.

## API

```cpp
#include "simditoa.h"

char buf[simditoa::MAX_DIGITS + 1];
size_t len = simditoa::to_chars(12345, buf);
buf[len] = '\0';
```

Both `int64_t` and `uint64_t` overloads are provided. The buffer must hold at least `simditoa::MAX_DIGITS + 1` (21) bytes.

### Heterogeneous and homogeneous variants

Two specialized conversion routines are exposed (paper §5.4 and §5.5). They produce identical output; the choice only affects performance on a batch of conversions.

- `simditoa::to_chars_heterogeneous` is branch-light and uses masked stores. Best when digit lengths in the batch vary unpredictably.
- `simditoa::to_chars_homogeneous` is per-length-specialized with direct unmasked stores. Best when most inputs share the same digit length (database identifiers, Unix timestamps, telemetry counters).

`simditoa::to_chars` defaults to the heterogeneous variant.

### Batch API with dynamic variant selection

For batch conversion, `to_chars_batch` automatically selects the right variant (paper §5.6, Algorithm 1) by sampling 1% of the input and checking whether one digit length dominates:

```cpp
#include "simditoa.h"

std::vector<uint64_t> values = /* ... */;
std::vector<std::array<char, simditoa::MAX_DIGITS + 1>> buffers(values.size());
std::vector<char*> ptrs(values.size());
std::vector<size_t> lengths(values.size());
for (size_t i = 0; i < values.size(); ++i) {
    ptrs[i] = buffers[i].data();
}

// Auto-selects homogeneous or heterogeneous based on the input distribution.
simditoa::to_chars_batch(values.data(), values.size(),
                         ptrs.data(), lengths.data());
```

To force a variant, pass it explicitly:

```cpp
simditoa::to_chars_batch(values.data(), values.size(),
                         ptrs.data(), lengths.data(),
                         simditoa::Variant::Homogeneous);
```

The sampling rate and homogeneity threshold are configurable via `simditoa::BatchOptions` (defaults: 0.01 and 0.95, matching the paper).

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

The library exposes both routines built on top of this kernel:

- **Heterogeneous** (§5.4): masked stores with a runtime-computed mask, uniform across digit lengths.
- **Homogeneous** (§5.5): a 20-way dispatcher on digit count, each branch using direct unmasked stores at compile-time offsets. The 9-15 digit branches use `_mm_bsrli_si128` to strip the leading-zero bytes from the 16-digit kernel output; the 17-20 digit branches write a 1-4 digit scalar prefix followed by a full-width 16-digit SIMD block (Figure 7 in the paper).

The dynamic selection step (§5.6) samples 1% of the input with a deterministic xorshift sampler and picks the variant whose strengths match the input's digit-length distribution.

## License

Dual-licensed under [MIT](LICENSE-MIT) and [Apache 2.0](LICENSE-APACHE).
