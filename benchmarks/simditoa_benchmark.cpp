#include "simditoa.h"

#include <benchmark/benchmark.h>
#include <fmt/format.h>
#include <jeaiii_to_text.h>
#include <rapidjson/internal/itoa.h>

extern "C" char *itoa_i64_yy(int64_t val, char *buf);

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <map>
#include <random>
#include <utility>
#include <vector>

namespace {

constexpr size_t NUM_VALUES = 1'000'000;

// Distribution selectors passed via state.range(0).
//   D_UNIFORM  : full int64 range, uniform. ~all values land in 18-19 digits.
//   D_LOG      : uniform over bit-widths [0, 63] then random sign. Equal
//                weight on each digit length.
//   D_FIXED    : positive values with exactly state.range(1) decimal digits.
enum Dist : int { D_UNIFORM = 0, D_LOG = 1, D_FIXED = 2 };

std::vector<int64_t> make_uniform() {
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
  std::vector<int64_t> v(NUM_VALUES);
  for (auto &x : v) {
    x = dist(rng);
  }
  return v;
}

std::vector<int64_t> make_log_uniform() {
  std::mt19937_64 rng(43);
  std::uniform_int_distribution<int> bits_dist(0, 63);
  std::vector<int64_t> v(NUM_VALUES);
  for (auto &x : v) {
    int b = bits_dist(rng);
    uint64_t mask = (b == 0) ? 0ULL : (~0ULL >> (64 - b));
    uint64_t mag = static_cast<uint64_t>(rng()) & mask;
    bool neg = (rng() & 1ULL) != 0;
    x = neg ? -static_cast<int64_t>(mag) : static_cast<int64_t>(mag);
  }
  return v;
}

std::vector<int64_t> make_fixed_length(int len) {
  std::mt19937_64 rng(static_cast<uint64_t>(44 + len));
  uint64_t pow10_lo = 1;
  for (int i = 1; i < len; ++i) {
    pow10_lo *= 10;
  }
  // For len == 1 we want 0..9; otherwise 10^(len-1)..10^len - 1.
  uint64_t lo = (len == 1) ? 0ULL : pow10_lo;
  uint64_t hi;
  if (len >= 19) {
    // 10^19 overflows int64; cap at INT64_MAX for 19-digit positives.
    hi = static_cast<uint64_t>(INT64_MAX);
  } else {
    hi = pow10_lo * 10 - 1;
  }
  std::uniform_int_distribution<uint64_t> dist(lo, hi);
  std::vector<int64_t> v(NUM_VALUES);
  for (auto &x : v) {
    x = static_cast<int64_t>(dist(rng));
  }
  return v;
}

const std::vector<int64_t> &get_values(int dist, int param) {
  static std::map<std::pair<int, int>, std::vector<int64_t>> cache;
  auto key = std::make_pair(dist, param);
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }
  std::vector<int64_t> v;
  switch (dist) {
  case D_UNIFORM:
    v = make_uniform();
    break;
  case D_LOG:
    v = make_log_uniform();
    break;
  case D_FIXED:
    v = make_fixed_length(param);
    break;
  }
  return cache.emplace(key, std::move(v)).first->second;
}

void set_counters(benchmark::State &state, size_t bytes) {
  state.counters["ints/s"] = benchmark::Counter(
      static_cast<double>(NUM_VALUES), benchmark::Counter::kIsIterationInvariantRate);
  state.counters["bytes/s"] = benchmark::Counter(
      static_cast<double>(bytes), benchmark::Counter::kIsRate,
      benchmark::Counter::OneK::kIs1000);
  state.counters["ns/int"] = benchmark::Counter(
      static_cast<double>(NUM_VALUES),
      benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
}

void bench_null(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  size_t bytes = 0;
  for (auto _ : state) {
    for (const auto &v : values) {
      int64_t x = v;
      benchmark::DoNotOptimize(x);
    }
    benchmark::ClobberMemory();
    bytes += NUM_VALUES * sizeof(int64_t);
  }
  set_counters(state, bytes);
}

void bench_std_to_chars(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  char buf[simditoa::MAX_DIGITS + 1];
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      auto res = std::to_chars(buf, buf + sizeof(buf), v);
      benchmark::DoNotOptimize(buf);
      iter_bytes += static_cast<size_t>(res.ptr - buf);
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

void bench_jeaiii_to_text(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  char buf[simditoa::MAX_DIGITS + 1];
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      char *end = jeaiii::to_text_from_integer(buf, v);
      benchmark::DoNotOptimize(buf);
      iter_bytes += static_cast<size_t>(end - buf);
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

void bench_yy_itoa(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  char buf[simditoa::MAX_DIGITS + 1];
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      char *end = itoa_i64_yy(v, buf);
      benchmark::DoNotOptimize(buf);
      iter_bytes += static_cast<size_t>(end - buf);
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

void bench_rapidjson_branchlut(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  char buf[simditoa::MAX_DIGITS + 1];
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      char *end = rapidjson::internal::i64toa(v, buf);
      benchmark::DoNotOptimize(buf);
      iter_bytes += static_cast<size_t>(end - buf);
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

void bench_fmt_format_int(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      fmt::format_int f(v);
      benchmark::DoNotOptimize(f.data());
      iter_bytes += f.size();
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

void bench_simditoa_to_chars(benchmark::State &state) {
  const auto &values = get_values(static_cast<int>(state.range(0)),
                                  static_cast<int>(state.range(1)));
  char buf[simditoa::MAX_DIGITS + 1];
  size_t bytes = 0;
  for (auto _ : state) {
    size_t iter_bytes = 0;
    for (const auto &v : values) {
      iter_bytes += simditoa::to_chars(v, buf);
      benchmark::DoNotOptimize(buf);
    }
    benchmark::ClobberMemory();
    bytes += iter_bytes;
  }
  set_counters(state, bytes);
}

auto max_stat = [](const std::vector<double> &v) {
  return *std::max_element(v.begin(), v.end());
};

}  // namespace

// Distribution sweep: every implementation runs against uniform and log-uniform.
#define REGISTER_DIST_SWEEP(fn)                              \
  BENCHMARK(fn)                                              \
      ->ArgNames({"dist", "param"})                          \
      ->Args({D_UNIFORM, 0})                                 \
      ->Args({D_LOG, 0})                                     \
      ->Repetitions(10)                                      \
      ->ComputeStatistics("max", max_stat)                   \
      ->DisplayAggregatesOnly(true)

REGISTER_DIST_SWEEP(bench_null);
REGISTER_DIST_SWEEP(bench_std_to_chars);
REGISTER_DIST_SWEEP(bench_jeaiii_to_text);
REGISTER_DIST_SWEEP(bench_yy_itoa);
REGISTER_DIST_SWEEP(bench_rapidjson_branchlut);
REGISTER_DIST_SWEEP(bench_fmt_format_int);
REGISTER_DIST_SWEEP(bench_simditoa_to_chars);

// Per-length sweep: only the three competitive implementations.
// 1/4/8/12/16/19 covers the small, medium, and large code paths.
#define REGISTER_LENGTH_SWEEP(fn)                            \
  BENCHMARK(fn)                                              \
      ->ArgNames({"dist", "len"})                            \
      ->Args({D_FIXED, 1})                                   \
      ->Args({D_FIXED, 4})                                   \
      ->Args({D_FIXED, 8})                                   \
      ->Args({D_FIXED, 12})                                  \
      ->Args({D_FIXED, 16})                                  \
      ->Args({D_FIXED, 19})                                  \
      ->Repetitions(10)                                      \
      ->ComputeStatistics("max", max_stat)                   \
      ->DisplayAggregatesOnly(true)

REGISTER_LENGTH_SWEEP(bench_jeaiii_to_text);
REGISTER_LENGTH_SWEEP(bench_yy_itoa);
REGISTER_LENGTH_SWEEP(bench_simditoa_to_chars);
