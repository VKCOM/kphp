#include <forward_list>
#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

#include "compiler/phpdoc.h"
#include "gtest/gtest_pred_impl.h"

TEST(phpdoc_test, parse_php_doc) {
  PhpDocComment doc(
    "**\n"
    " * @kphp-infer\n"
    " *\n");
  ASSERT_TRUE(doc.tags.begin() != doc.tags.end());
  ASSERT_EQ(doc.tags.front().type, PhpDocType::kphp_infer);
  ASSERT_EQ(doc.tags.front().get_tag_name(), "@kphp-infer");
  ASSERT_TRUE(doc.tags.front().value_as_string().empty());
}
