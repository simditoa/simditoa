#include "simditoa.h"
#include "simditoa/implementation_detection.h"
#include "simditoa/portability.h"

static_assert(SIMDITOA_IS_RISCV == 1, "expected RISC-V target");
static_assert(SIMDITOA_IS_RISCV64 == 1, "expected RV64 target");
static_assert(SIMDITOA_IS_X86_64 == 0, "RISC-V smoke build must not keep x86_64");
static_assert(SIMDITOA_IMPLEMENTATION_AVX512 == 0,
              "RISC-V smoke build must not enable AVX-512");
static_assert(SIMDITOA_IMPLEMENTATION_FALLBACK == 1,
              "RISC-V smoke build must use the fallback implementation");

void riscv_compile_smoke() {
  char buffer_a[simditoa::MAX_DIGITS + 1] = {};
  char buffer_b[simditoa::MAX_DIGITS + 1] = {};
  char *outputs[2] = {buffer_a, buffer_b};
  size_t lengths[2] = {};
  const uint64_t values[2] = {1, 42};

  (void)simditoa::to_chars(uint64_t{123456789}, buffer_a);
  (void)simditoa::to_chars(int64_t{-123456789}, buffer_b);
  simditoa::to_chars_batch(values, 2, outputs, lengths);
}
