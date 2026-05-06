#include "simditoa.h"

#include <benchmark/benchmark.h>
#include <fmt/format.h>
#include <jeaiii_to_text.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <random>
#include <vector>

namespace {

constexpr size_t NUM_VALUES = 1'000'000;

const std::vector<int64_t> &get_values() {
  static const std::vector<int64_t> values = [] {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
    std::vector<int64_t> v(NUM_VALUES);
    for (auto &x : v) {
      x = dist(rng);
    }
    return v;
  }();
  return values;
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
  const auto &values = get_values();
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
  const auto &values = get_values();
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
  const auto &values = get_values();
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

void bench_fmt_format_int(benchmark::State &state) {
  const auto &values = get_values();
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
  const auto &values = get_values();
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

BENCHMARK(bench_null)
    ->Repetitions(10)
    ->ComputeStatistics("max", max_stat)
    ->DisplayAggregatesOnly(true);

BENCHMARK(bench_std_to_chars)
    ->Repetitions(10)
    ->ComputeStatistics("max", max_stat)
    ->DisplayAggregatesOnly(true);

BENCHMARK(bench_jeaiii_to_text)
    ->Repetitions(10)
    ->ComputeStatistics("max", max_stat)
    ->DisplayAggregatesOnly(true);

BENCHMARK(bench_fmt_format_int)
    ->Repetitions(10)
    ->ComputeStatistics("max", max_stat)
    ->DisplayAggregatesOnly(true);

BENCHMARK(bench_simditoa_to_chars)
    ->Repetitions(10)
    ->ComputeStatistics("max", max_stat)
    ->DisplayAggregatesOnly(true);
