#include "simditoa.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

static constexpr size_t NUM_VALUES = 1'000'000;
static constexpr int ITERATIONS = 10;

int main() {
  // Generate random int64 values
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
  std::vector<int64_t> values(NUM_VALUES);
  for (auto &v : values) {
    v = dist(rng);
  }

  char buf[simditoa::MAX_DIGITS + 1];
  size_t total_chars = 0;

  // ---- Benchmark: std::to_chars (baseline) ----

  std::printf("std::to_chars (baseline)\n");

  // Warm up
  for (const auto &v : values) {
    auto res = std::to_chars(buf, buf + sizeof(buf), v);
    total_chars += static_cast<size_t>(res.ptr - buf);
  }

  double baseline_best_ns = 1e18;
  for (int iter = 0; iter < ITERATIONS; ++iter) {
    total_chars = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto &v : values) {
      auto res = std::to_chars(buf, buf + sizeof(buf), v);
      total_chars += static_cast<size_t>(res.ptr - buf);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count();
    double ns_per_int = ns / static_cast<double>(NUM_VALUES);
    if (ns < baseline_best_ns) {
      baseline_best_ns = ns;
    }
    std::printf("  iter %2d: %.2f ns/int  (%.2f M ints/sec)\n", iter + 1,
                ns_per_int, static_cast<double>(NUM_VALUES) / ns * 1e3);
  }

  double baseline_best_per_int =
      baseline_best_ns / static_cast<double>(NUM_VALUES);
  std::printf("\n  best: %.2f ns/int  (%.2f M ints/sec)\n",
              baseline_best_per_int,
              static_cast<double>(NUM_VALUES) / baseline_best_ns * 1e3);
  std::printf("  total chars (last run): %zu\n\n", total_chars);

  // ---- Benchmark: simditoa::to_chars ----

  std::printf("simditoa::to_chars\n");

  // Warm up
  total_chars = 0;
  for (const auto &v : values) {
    total_chars += simditoa::to_chars(v, buf);
  }

  double simditoa_best_ns = 1e18;
  for (int iter = 0; iter < ITERATIONS; ++iter) {
    total_chars = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto &v : values) {
      total_chars += simditoa::to_chars(v, buf);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count();
    double ns_per_int = ns / static_cast<double>(NUM_VALUES);
    if (ns < simditoa_best_ns) {
      simditoa_best_ns = ns;
    }
    std::printf("  iter %2d: %.2f ns/int  (%.2f M ints/sec)\n", iter + 1,
                ns_per_int, static_cast<double>(NUM_VALUES) / ns * 1e3);
  }

  double simditoa_best_per_int =
      simditoa_best_ns / static_cast<double>(NUM_VALUES);
  std::printf("\n  best: %.2f ns/int  (%.2f M ints/sec)\n",
              simditoa_best_per_int,
              static_cast<double>(NUM_VALUES) / simditoa_best_ns * 1e3);
  std::printf("  total chars (last run): %zu\n\n", total_chars);

  // ---- Summary ----

  std::printf("--- Summary ---\n");
  std::printf("  std::to_chars:    %.2f ns/int\n", baseline_best_per_int);
  std::printf("  simditoa:         %.2f ns/int\n", simditoa_best_per_int);
  std::printf("  speedup:          %.2fx\n",
              baseline_best_ns / simditoa_best_ns);

  return 0;
}
