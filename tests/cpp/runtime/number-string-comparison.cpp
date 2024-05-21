#include <gtest/gtest.h>

#include "kphp-core/kphp_core.h"

template <typename T>
struct TestCaseComparison {
  T number;
  const char *str;
  bool less;
  bool equal;
  bool greater;

  bool php7_less;
  bool php7_equal;
  bool php7_greater;
};

template<typename T>
void run_test_cases(const std::vector<TestCaseComparison<T>> &cases);

TEST(number_string_comparison_test, test_comp_int) {
  const auto cases = std::vector<TestCaseComparison<int64_t>>{
    //               less   equal  greater  php7 same
    { 0,   "0",      false, true,  false,        false, true, false },

    { 0,   "0",      false, true,  false,        false, true, false },
    { 0,   "0.0",    false, true,  false,        false, true, false },
    { 42,  "  42",   false, true,  false,        false, true, false },
    { 100, "1e2",    false, true,  false,        false, true, false },

    { 0,   "foo",    true,  false, false,        false, true, false },
    { 0,   "",       false, false, true,         false, true, false },
    { 42,  "42foo",  true,  false, false,        false, true, false },

    { 100, "1e2foo", true,  false, false,        false, true, false },
    { 10,  "10..",   true,  false, false,        false, true, false },
    { 10,  "10e",    true,  false, false,        false, true, false },
  };

  run_test_cases<int64_t>(cases);
}

TEST(number_string_comparison_test, test_comp_double) {
  const auto cases = std::vector<TestCaseComparison<double>>{
    //                   less   equal  greater  php7 same
    { 0.0,   "0",        false, true,  false,        false, true, false },
    { 0.0,   "0.0",      false, true,  false,        false, true, false },
    { 0.0,   "foo",      true,  false, false,        false, true, false },
    { 0.0,   "",         false, false, true,         false, true, false },
    { 42.56, "  42.56",  false, true,  false,        false, true, false },
    { 42.56, "42.56foo", true,  false, false,        false, true, false },

    { 1e2,   "1e2",      false, true,  false,        false, true, false },
    { 1e2,   "1e2foo",   true,  false, false,        false, true, false },

    { 10,    "10.56.",   true,  false, false,        true,  false, false },
    { 10,    "10.e10",   true,  false, false,        true,  false, false },
  };

  run_test_cases<double>(cases);
}

template<typename T>
void run_test_cases(const std::vector<TestCaseComparison<T>> &cases) {
  for (int i = 0; i < cases.size(); ++i) {
    const auto &test = cases[i];

    const auto less_res = less_number_string_as_php8_impl(test.number, string(test.str));
    const auto equal_res = eq2_number_string_as_php8(test.number, string(test.str));
    // '$a < $b' === '$b > $a'
    const auto greater_res = less_string_number_as_php8_impl(string(test.str), test.number);

    ASSERT_EQ(less_res, test.less) << "Case " << i;
    ASSERT_EQ(greater_res, test.greater) << "Case " << i;
    ASSERT_EQ(equal_res, test.equal) << "Case " << i;

    const auto php7_less_res = lt(test.number, string(test.str));
    const auto php7_equal_res = eq2(test.number, string(test.str));
    // '$a < $b' === '$b > $a'
    const auto php7_greater_res = lt(string(test.str), test.number);

    ASSERT_EQ(php7_less_res, test.php7_less) << "Case " << i;
    ASSERT_EQ(php7_greater_res, test.php7_greater) << "Case " << i;
    ASSERT_EQ(php7_equal_res, test.php7_equal) << "Case " << i;
  }
}
