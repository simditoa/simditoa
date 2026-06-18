#include "simditoa.h"

#include <cassert>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

using ConvFn = size_t (*)(uint64_t, char *) noexcept;
using SignedConvFn = size_t (*)(int64_t, char *) noexcept;

struct Variant {
  const char *name;
  ConvFn unsigned_fn;
  SignedConvFn signed_fn;
};

const Variant kVariants[] = {
    {"to_chars",
     static_cast<ConvFn>(&simditoa::to_chars),
     static_cast<SignedConvFn>(&simditoa::to_chars)},
    {"to_chars_heterogeneous",
     static_cast<ConvFn>(&simditoa::to_chars_heterogeneous),
     static_cast<SignedConvFn>(&simditoa::to_chars_heterogeneous)},
    {"to_chars_homogeneous",
     static_cast<ConvFn>(&simditoa::to_chars_homogeneous),
     static_cast<SignedConvFn>(&simditoa::to_chars_homogeneous)},
};

std::string convert_unsigned(const Variant &v, uint64_t value) {
  char buf[simditoa::MAX_DIGITS + 1];
  std::memset(buf, 0, sizeof(buf));
  size_t len = v.unsigned_fn(value, buf);
  return std::string(buf, len);
}

std::string convert_signed(const Variant &v, int64_t value) {
  char buf[simditoa::MAX_DIGITS + 1];
  std::memset(buf, 0, sizeof(buf));
  size_t len = v.signed_fn(value, buf);
  return std::string(buf, len);
}

void check(const Variant &v, uint64_t value, const std::string &expected) {
  std::string got = convert_unsigned(v, value);
  if (got != expected) {
    std::cerr << "FAIL [" << v.name << "]: to_chars(" << value << ") = \""
              << got << "\", expected \"" << expected << "\"\n";
    std::abort();
  }
}

void check_signed(const Variant &v, int64_t value, const std::string &expected) {
  std::string got = convert_signed(v, value);
  if (got != expected) {
    std::cerr << "FAIL [" << v.name << "]: to_chars(" << value << ") = \""
              << got << "\", expected \"" << expected << "\"\n";
    std::abort();
  }
}

void test_basic_for(const Variant &v) {
  check_signed(v, 0, "0");
  check_signed(v, 1, "1");
  check_signed(v, 9, "9");
  check_signed(v, 10, "10");
  check_signed(v, 42, "42");
  check_signed(v, 99, "99");
  check_signed(v, 100, "100");
  check_signed(v, 999, "999");
  check_signed(v, 1000, "1000");
  check_signed(v, 9999, "9999");
  check_signed(v, 10000, "10000");
  check_signed(v, 12345, "12345");
  check_signed(v, 1000000, "1000000");
}

void test_negative_for(const Variant &v) {
  check_signed(v, -1, "-1");
  check_signed(v, -9, "-9");
  check_signed(v, -10, "-10");
  check_signed(v, -42, "-42");
  check_signed(v, -100, "-100");
  check_signed(v, -12345, "-12345");
  check_signed(v, -1000000, "-1000000");
}

void test_edge_cases_for(const Variant &v) {
  check_signed(v, INT64_MAX, "9223372036854775807");
  check_signed(v, INT64_MIN, "-9223372036854775808");
  check(v, UINT64_MAX, "18446744073709551615");
  check(v, 0, "0");
  check(v, 1, "1");
}

void test_powers_of_10_for(const Variant &v) {
  uint64_t power = 1;
  for (int i = 0; i < 19; ++i) {
    check(v, power, std::to_string(power));
    check(v, power - 1, std::to_string(power - 1));
    check(v, power + 1, std::to_string(power + 1));
    power *= 10;
  }
  check(v, power, std::to_string(power));
}

void test_all_digit_lengths_for(const Variant &v) {
  // Two values for every digit length 1..20 inclusive, hitting the per-length
  // homogeneous paths.
  uint64_t values[] = {
      0,
      1,
      5,
      9,
      10,
      50,
      99,
      100,
      500,
      999,
      1000,
      5000,
      9999,
      10000,
      50000,
      99999,
      100000,
      500000,
      999999,
      1000000,
      5000000,
      9999999,
      10000000,
      50000000,
      99999999,
      100000000,
      500000000,
      999999999,
      1000000000ULL,
      9999999999ULL,
      10000000000ULL,
      99999999999ULL,
      100000000000ULL,
      999999999999ULL,
      1000000000000ULL,
      9999999999999ULL,
      10000000000000ULL,
      99999999999999ULL,
      100000000000000ULL,
      999999999999999ULL,
      1000000000000000ULL,
      9999999999999999ULL,
      10000000000000000ULL,
      99999999999999999ULL,
      100000000000000000ULL,
      999999999999999999ULL,
      1000000000000000000ULL,
      9999999999999999999ULL,
      10000000000000000000ULL,
      UINT64_MAX,
  };
  for (uint64_t value : values) {
    check(v, value, std::to_string(value));
  }
}

void test_random_for(const Variant &v) {
  std::mt19937_64 rng(12345);
  std::uniform_int_distribution<uint64_t> udist(0, UINT64_MAX);
  for (int i = 0; i < 50000; ++i) {
    uint64_t value = udist(rng);
    check(v, value, std::to_string(value));
  }
  std::uniform_int_distribution<int64_t> sdist(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 50000; ++i) {
    int64_t value = sdist(rng);
    check_signed(v, value, std::to_string(value));
  }
}

void test_buffer_isolation_for(const Variant &v) {
  // Verify the variant never writes past MAX_DIGITS bytes from the start
  // of the buffer (a contractual upper bound the homogeneous variant can
  // approach because of its full-width direct stores).
  constexpr size_t kPad = 8;
  char buf[kPad + simditoa::MAX_DIGITS + 1 + kPad];
  for (int n = 1; n <= 20; ++n) {
    std::memset(buf, 0x5A, sizeof(buf));
    uint64_t value = 1;
    for (int i = 1; i < n; ++i) {
      value = value * 10 + 1;
    }
    char *start = buf + kPad;
    size_t len = v.unsigned_fn(value, start);
    if (len != static_cast<size_t>(n)) {
      std::cerr << "FAIL [" << v.name << "]: length mismatch for n=" << n
                << " value=" << value << " got len=" << len << "\n";
      std::abort();
    }
    for (size_t i = 0; i < kPad; ++i) {
      if (buf[i] != 0x5A) {
        std::cerr << "FAIL [" << v.name
                  << "]: write before buffer at offset " << i
                  << " for n=" << n << "\n";
        std::abort();
      }
      if (buf[kPad + simditoa::MAX_DIGITS + 1 + i] != 0x5A) {
        std::cerr << "FAIL [" << v.name << "]: write past MAX_DIGITS+1 for n="
                  << n << "\n";
        std::abort();
      }
    }
  }
}

// ---- Batch API and dynamic selection ----

void test_select_variant_homogeneous() {
  std::vector<uint64_t> values(1000);
  std::mt19937_64 rng(7);
  std::uniform_int_distribution<uint64_t> tail(1000000ULL, 9999999ULL);
  for (auto &x : values) {
    x = tail(rng); // all 7 digits
  }
  simditoa::Variant chosen = simditoa::select_variant(values.data(), values.size());
  if (chosen != simditoa::Variant::Homogeneous) {
    std::cerr << "FAIL: select_variant did not pick Homogeneous for uniform "
                 "7-digit input\n";
    std::abort();
  }
}

void test_select_variant_heterogeneous() {
  std::vector<uint64_t> values;
  values.reserve(2000);
  std::mt19937_64 rng(11);
  // Mix of 1-digit and 19-digit values, 50/50.
  std::uniform_int_distribution<uint64_t> small(0ULL, 9ULL);
  std::uniform_int_distribution<uint64_t> large(1000000000000000000ULL,
                                                9999999999999999999ULL);
  for (int i = 0; i < 1000; ++i) {
    values.push_back(small(rng));
    values.push_back(large(rng));
  }
  simditoa::Variant chosen = simditoa::select_variant(values.data(), values.size());
  if (chosen != simditoa::Variant::Heterogeneous) {
    std::cerr << "FAIL: select_variant did not pick Heterogeneous for 1/19 "
                 "digit mix\n";
    std::abort();
  }
}

void test_select_variant_edge_cases() {
  if (simditoa::select_variant(nullptr, 0) != simditoa::Variant::Heterogeneous) {
    std::cerr << "FAIL: select_variant on empty input must return Heterogeneous\n";
    std::abort();
  }
  uint64_t single = 42;
  if (simditoa::select_variant(&single, 1) != simditoa::Variant::Homogeneous) {
    std::cerr << "FAIL: select_variant on single value must return Homogeneous\n";
    std::abort();
  }
}

void test_batch_explicit_variant() {
  std::vector<uint64_t> values;
  std::mt19937_64 rng(99);
  std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
  for (int i = 0; i < 5000; ++i) {
    values.push_back(dist(rng));
  }

  // Per-element output buffer.
  std::vector<std::array<char, simditoa::MAX_DIGITS + 1>> bufs(values.size());
  std::vector<char *> ptrs(values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    bufs[i].fill(0);
    ptrs[i] = bufs[i].data();
  }
  std::vector<size_t> lens(values.size(), 0);

  for (simditoa::Variant variant :
       {simditoa::Variant::Homogeneous, simditoa::Variant::Heterogeneous}) {
    for (auto &b : bufs) {
      b.fill(0);
    }
    simditoa::to_chars_batch(values.data(), values.size(), ptrs.data(),
                             lens.data(), variant);
    for (size_t i = 0; i < values.size(); ++i) {
      std::string got(ptrs[i], lens[i]);
      std::string expected = std::to_string(values[i]);
      if (got != expected) {
        std::cerr << "FAIL: batch variant=" << static_cast<int>(variant)
                  << " idx=" << i << " got=\"" << got << "\" expected=\""
                  << expected << "\"\n";
        std::abort();
      }
    }
  }
}

void test_batch_dynamic_selection() {
  // Homogeneous batch: all 10-digit values.
  std::vector<uint64_t> values(2000);
  std::mt19937_64 rng(2026);
  std::uniform_int_distribution<uint64_t> dist(1000000000ULL, 9999999999ULL);
  for (auto &x : values) {
    x = dist(rng);
  }
  std::vector<std::array<char, simditoa::MAX_DIGITS + 1>> bufs(values.size());
  std::vector<char *> ptrs(values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    bufs[i].fill(0);
    ptrs[i] = bufs[i].data();
  }
  std::vector<size_t> lens(values.size(), 0);

  simditoa::to_chars_batch(values.data(), values.size(), ptrs.data(),
                           lens.data());
  for (size_t i = 0; i < values.size(); ++i) {
    std::string got(ptrs[i], lens[i]);
    std::string expected = std::to_string(values[i]);
    if (got != expected) {
      std::cerr << "FAIL: dynamic batch idx=" << i << " got=\"" << got
                << "\" expected=\"" << expected << "\"\n";
      std::abort();
    }
  }
}

} // namespace

int main() {
  for (const Variant &v : kVariants) {
    test_basic_for(v);
    test_negative_for(v);
    test_edge_cases_for(v);
    test_powers_of_10_for(v);
    test_all_digit_lengths_for(v);
    test_random_for(v);
    test_buffer_isolation_for(v);
  }

  test_select_variant_homogeneous();
  test_select_variant_heterogeneous();
  test_select_variant_edge_cases();
  test_batch_explicit_variant();
  test_batch_dynamic_selection();

  std::cout << "All tests passed (3 variants x 100,000 random + edge cases + "
               "batch API)."
            << std::endl;
  return 0;
}
