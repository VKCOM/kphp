// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>

#include <exception>
#include <stdexcept>

#include "list_of_types.h"

using list = vk::list_of_types<std::runtime_error, std::exception, double>;

TEST(list_of_types_tests, in_list) {
  EXPECT_TRUE(list::in_list<double>());
  EXPECT_FALSE(list::in_list<std::logic_error>());
}

TEST(list_of_types_tests, base_in_list) {
  EXPECT_TRUE(list::base_in_list<std::logic_error>());
  EXPECT_TRUE(list::base_in_list<std::exception>());
  EXPECT_FALSE(list::base_in_list<double>());
  EXPECT_FALSE(list::base_in_list<int>());
}
