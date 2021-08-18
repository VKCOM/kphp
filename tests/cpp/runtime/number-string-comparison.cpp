#include <gtest/gtest.h>

#include "runtime/kphp_core.h"

template <typename T>
struct TestCaseComparison {
  T number;
  const char *str;
  bool res;
};

TEST(number_string_comparison_test, test_comp_int) {
  const auto cases = std::vector<TestCaseComparison<int64_t>>{
    {0,   "0",      true},
    {0,   "0.0",    true},
    {0,   "foo",    false},
    {0,   "",       false},
    {42,  "  42",   true},
    {42,  "42foo",  false},

    {100, "1e2",    true},
    {100, "1e2foo", false},

    {10,  "10..",   false},
    {10,  "10e",    false},
  };

  for (int i = 0; i < cases.size(); ++i) {
    const auto &test = cases[i];

    ASSERT_EQ(compare_number_string_as_php8(test.number, string(test.str)), test.res) << "Case " << i;
  }
}

TEST(number_string_comparison_test, test_comp_double) {
  const auto cases = std::vector<TestCaseComparison<double>>{
    {0.0,   "0",        true},
    {0.0,   "0.0",      true},
    {0.0,   "foo",      false},
    {0.0,   "",         false},
    {42.56, "  42.56",  true},
    {42.56, "42.56foo", false},

    {1e2,   "1e2",      true},
    {1e2,   "1e2foo",   false},

    {10,    "10.56.",   false},
    {10,    "10.e10",   false},
  };

  for (int i = 0; i < cases.size(); ++i) {
    const auto &test = cases[i];

    ASSERT_EQ(compare_number_string_as_php8(test.number, string(test.str)), test.res) << "Case " << i;
  }
}
