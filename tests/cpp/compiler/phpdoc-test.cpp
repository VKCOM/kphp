#include <gtest/gtest.h>

#include "compiler/phpdoc.h"

TEST(phpdoc_test, parse_php_doc) {
  auto parsing_result = parse_php_doc(
    "**\n"
    " * @kphp-infer\n"
    " *\n");
  ASSERT_EQ(parsing_result.size(), 1);
  ASSERT_EQ(parsing_result.front().type, php_doc_tag::kphp_infer);
  ASSERT_EQ(parsing_result.front().name, "@kphp-infer");
  ASSERT_TRUE(parsing_result.front().value.empty());
  ASSERT_EQ(parsing_result.front().line_num, -1);
}
