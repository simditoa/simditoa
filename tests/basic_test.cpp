#include "simditoa.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <random>

static std::string convert_signed(int64_t value) {
  char buf[simditoa::MAX_DIGITS + 1];
  size_t len = simditoa::to_chars(value, buf);
  return std::string(buf, len);
}

static std::string convert_unsigned(uint64_t value) {
  char buf[simditoa::MAX_DIGITS + 1];
  size_t len = simditoa::to_chars(value, buf);
  return std::string(buf, len);
}

static void test_basic() {
  assert(convert_signed(0) == "0");
  assert(convert_signed(1) == "1");
  assert(convert_signed(9) == "9");
  assert(convert_signed(10) == "10");
  assert(convert_signed(42) == "42");
  assert(convert_signed(99) == "99");
  assert(convert_signed(100) == "100");
  assert(convert_signed(999) == "999");
  assert(convert_signed(1000) == "1000");
  assert(convert_signed(9999) == "9999");
  assert(convert_signed(10000) == "10000");
  assert(convert_signed(12345) == "12345");
  assert(convert_signed(1000000) == "1000000");
}

static void test_negative() {
  assert(convert_signed(-1) == "-1");
  assert(convert_signed(-9) == "-9");
  assert(convert_signed(-10) == "-10");
  assert(convert_signed(-42) == "-42");
  assert(convert_signed(-100) == "-100");
  assert(convert_signed(-12345) == "-12345");
  assert(convert_signed(-1000000) == "-1000000");
}

static void test_edge_cases() {
  assert(convert_signed(INT64_MAX) == "9223372036854775807");
  assert(convert_signed(INT64_MIN) == "-9223372036854775808");
  assert(convert_unsigned(UINT64_MAX) == "18446744073709551615");
  assert(convert_unsigned(0) == "0");
  assert(convert_unsigned(1) == "1");
}

static void test_powers_of_10() {
  uint64_t power = 1;
  for (int i = 0; i < 19; ++i) {
    assert(convert_unsigned(power) == std::to_string(power));
    assert(convert_unsigned(power - 1) == std::to_string(power - 1));
    assert(convert_unsigned(power + 1) == std::to_string(power + 1));
    power *= 10;
  }
  // 10^19
  assert(convert_unsigned(power) == std::to_string(power));
}

static void test_all_digit_lengths() {
  // Test representative values for every digit count (1-20)
  uint64_t values[] = {
      0,
      1,
      5,
      9, // 1 digit
      10,
      50,
      99, // 2 digits
      100,
      500,
      999, // 3 digits
      1000,
      5000,
      9999, // 4 digits
      10000,
      50000,
      99999, // 5 digits
      100000,
      500000,
      999999, // 6 digits
      1000000,
      5000000,
      9999999, // 7 digits
      10000000,
      50000000,
      99999999, // 8 digits
      100000000,
      500000000,
      999999999, // 9 digits
      1000000000ULL,
      9999999999ULL, // 10 digits
      10000000000ULL,
      99999999999ULL, // 11 digits
      100000000000ULL,
      999999999999ULL, // 12 digits
      1000000000000ULL,
      9999999999999ULL, // 13 digits
      10000000000000ULL,
      99999999999999ULL, // 14 digits
      100000000000000ULL,
      999999999999999ULL, // 15 digits
      1000000000000000ULL,
      9999999999999999ULL, // 16 digits
      10000000000000000ULL,
      99999999999999999ULL, // 17 digits
      100000000000000000ULL,
      999999999999999999ULL, // 18 digits
      1000000000000000000ULL,
      9999999999999999999ULL, // 19 digits
      10000000000000000000ULL,
      UINT64_MAX // 20 digits
  };
  for (uint64_t v : values) {
    std::string expected = std::to_string(v);
    std::string got = convert_unsigned(v);
    if (got != expected) {
      std::cerr << "FAIL: to_chars(" << v << ") = \"" << got
                << "\", expected \"" << expected << "\"" << std::endl;
      std::abort();
    }
  }
}

static void test_random() {
  std::mt19937_64 rng(12345);
  std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

  for (int i = 0; i < 100000; ++i) {
    uint64_t v = dist(rng);
    std::string expected = std::to_string(v);
    std::string got = convert_unsigned(v);
    if (got != expected) {
      std::cerr << "FAIL: to_chars(" << v << ") = \"" << got
                << "\", expected \"" << expected << "\"" << std::endl;
      std::abort();
    }
  }

  // Also test signed random values
  std::uniform_int_distribution<int64_t> sdist(INT64_MIN, INT64_MAX);
  for (int i = 0; i < 100000; ++i) {
    int64_t v = sdist(rng);
    std::string expected = std::to_string(v);
    std::string got = convert_signed(v);
    if (got != expected) {
      std::cerr << "FAIL: to_chars(" << v << ") = \"" << got
                << "\", expected \"" << expected << "\"" << std::endl;
      std::abort();
    }
  }
}

int main() {
  test_basic();
  test_negative();
  test_edge_cases();
  test_powers_of_10();
  test_all_digit_lengths();
  test_random();

  std::cout << "All tests passed (200,000 random + edge cases)." << std::endl;
  return 0;
}
