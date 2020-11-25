#include <gtest/gtest.h>

#include "compiler/data/performance-inspections.h"

using PI = PerformanceInspections;

TEST(performance_inspections_test, test_parse) {
  PI inspections;
  ASSERT_EQ(inspections.inspections(), 0);

  inspections.add_from_php_doc("all");
  ASSERT_EQ(inspections.inspections(), PI::all_inspections);

  inspections = PI{};
  inspections.add_from_php_doc("implicit-array-cast");
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  inspections = PI{};
  inspections.add_from_php_doc("all !implicit-array-cast");
  ASSERT_EQ(inspections.inspections(), PI::array_merge_into | PI::array_reserve | PI::constant_execution_in_loop);

  inspections = PI{};
  inspections.add_from_php_doc("!all implicit-array-cast");
  ASSERT_EQ(inspections.inspections(), 0);

  inspections = PI{};
  inspections.add_from_php_doc("!array-merge-into array-merge-into implicit-array-cast");
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);
}

TEST(performance_inspections_test, test_merge) {
  PI caller_inspections;
  caller_inspections.add_from_php_doc("implicit-array-cast");

  PI inspections;
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::ok, PI::no_inspections));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  caller_inspections = {};
  caller_inspections.add_from_php_doc("!implicit-array-cast");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::conflict, PI::implicit_array_cast));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  caller_inspections = {};
  caller_inspections.add_from_php_doc("!array-merge-into");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::ok, PI::no_inspections));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  caller_inspections = {};
  caller_inspections.add_from_php_doc("array-merge-into");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::conflict, PI::array_merge_into));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  caller_inspections = {};
  caller_inspections.add_from_php_doc("all");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::conflict, PI::array_merge_into));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  ASSERT_EQ(inspections.merge_with_caller(PI{}), std::make_pair(PI::InheritStatus::ok, PI::no_inspections));
  ASSERT_EQ(inspections.inspections(), PI::implicit_array_cast);

  inspections = {};
  caller_inspections.add_from_php_doc("all !implicit-array-cast");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::ok, PI::no_inspections));
  ASSERT_EQ(inspections.inspections(), PI::array_merge_into | PI::array_reserve | PI::constant_execution_in_loop);

  caller_inspections = {};
  caller_inspections.add_from_php_doc("all");
  ASSERT_EQ(inspections.merge_with_caller(caller_inspections), std::make_pair(PI::InheritStatus::conflict, PI::implicit_array_cast));
}

TEST(performance_inspections_test, test_bad_parse) {
  PI inspections;

  ASSERT_THROW(inspections.add_from_php_doc("abc"), std::runtime_error);
  ASSERT_THROW(inspections.add_from_php_doc("  "), std::runtime_error);
  ASSERT_THROW(inspections.add_from_php_doc("!"), std::runtime_error);
  ASSERT_EQ(inspections.inspections(), 0);
}
