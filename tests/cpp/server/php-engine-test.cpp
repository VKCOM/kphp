#include <gtest/gtest.h>

#include "common/kprintf.h"

#include "server/php-engine-vars.h"

TEST(php_engine_test, test_init_logname) {
  void init_logname(const char *src);

  init_logname("foo");
  ASSERT_STREQ(logname, "foo");
  ASSERT_FALSE(logname_pattern);

  init_logname("foo-bar-baz");
  ASSERT_STREQ(logname, "foo-bar-baz");
  ASSERT_FALSE(logname_pattern);

  init_logname("foo%");
  ASSERT_STREQ(logname, "foo");
  ASSERT_STREQ(logname_pattern, "foo%d");

  init_logname("foo%%");
  ASSERT_STREQ(logname, "foo");
  ASSERT_STREQ(logname_pattern, "foo%d");

  init_logname("foo%bar%baz");
  ASSERT_STREQ(logname, "foobarbaz");
  ASSERT_STREQ(logname_pattern, "foo%dbarbaz");

  init_logname("foo-%");
  ASSERT_STREQ(logname, "foo");
  ASSERT_STREQ(logname_pattern, "foo-%d");

  init_logname("foo-%%");
  ASSERT_STREQ(logname, "foo");
  ASSERT_STREQ(logname_pattern, "foo-%d");

  init_logname("foo-%bar%baz");
  ASSERT_STREQ(logname, "foobarbaz");
  ASSERT_STREQ(logname_pattern, "foo-%dbarbaz");

  init_logname("foo-%-%");
  ASSERT_STREQ(logname, "foo-");
  ASSERT_STREQ(logname_pattern, "foo-%d-");

  init_logname("foo-%bar-%baz");
  ASSERT_STREQ(logname, "foobar-baz");
  ASSERT_STREQ(logname_pattern, "foo-%dbar-baz");
}
