#include <gtest/gtest.h>

#include "runtime/confdata-functions.h"
#include "runtime/confdata-global-manager.h"

namespace {

void init_global_confdata_confdata() {
  static bool initiated = false;
  if (initiated) {
    return;
  }
  initiated = true;

  pid = 0;
  std::forward_list<vk::string_view> force_ignore_prefixes;
  auto &global_manager = ConfdataGlobalManager::get();
  global_manager.init(1024 * 1024 * 16, std::unordered_set<vk::string_view>{}, nullptr, std::move(force_ignore_prefixes));
  auto confdata_sample_storage = global_manager.get_current().get_confdata();

  confdata_sample_storage[string{"_key_1"}] = string{"value_1"};
  confdata_sample_storage[string{"_key_2"}] = string{"value_2"};

  confdata_sample_storage[string{"_one dot."}] = array<mixed>{
    std::make_pair(mixed{string{"one_1"}}, mixed{string{"one_value_1"}}),
    std::make_pair(mixed{string{"one_2"}}, mixed{string{"one_value_2"}}),
    std::make_pair(mixed{3}, mixed{string{"one_value_3"}}),
    std::make_pair(mixed{string{"one_4"}}, mixed{string{"one_value_4"}})
  };

  confdata_sample_storage[string{"_two dot.a."}] = array<mixed>{
    std::make_pair(mixed{string{"two_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{"two_2a"}}, mixed{string{"a_one_value_2"}}),
  };
  confdata_sample_storage[string{"_two dot.b."}] = array<mixed>{
    std::make_pair(mixed{string{"two_1b"}}, mixed{string{"b_one_value_1"}}),
    std::make_pair(mixed{string{"two_2b"}}, mixed{string{"b_one_value_2"}}),
  };
  confdata_sample_storage[string{"_two dot."}] = array<mixed>{
    std::make_pair(mixed{string{"a.two_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{"a.two_2a"}}, mixed{string{"a_one_value_2"}}),
    std::make_pair(mixed{string{"b.two_1b"}}, mixed{string{"b_one_value_1"}}),
    std::make_pair(mixed{string{"b.two_2b"}}, mixed{string{"b_one_value_2"}}),
  };

  global_manager.get_current().reset(std::move(confdata_sample_storage));

  init_confdata_functions_lib();
}

} // namespace

TEST(confdata_functions_test, test_confdata_get_value_unknown_key) {
  init_global_confdata_confdata();

  for (auto unknown_key: {"", "unknown_key", "_key_3",
                          "_one dot.1", "_one dot.one_3", "_one dot.one_5",
                          "_two dot.a.one_3a", "_two dot.b.one_3b", "_two dot.c.one_1c"}) {
    ASSERT_TRUE(f$confdata_get_value(string{unknown_key}).is_null());
  }
}

TEST(confdata_functions_test, test_confdata_get_value) {
  init_global_confdata_confdata();

  ASSERT_TRUE(equals(f$confdata_get_value(string{"_key_1"}), string{"value_1"}));
  ASSERT_TRUE(equals(f$confdata_get_value(string{"_one dot.one_2"}), string{"one_value_2"}));
  ASSERT_TRUE(equals(f$confdata_get_value(string{"_one dot.3"}), string{"one_value_3"}));
  ASSERT_TRUE(equals(f$confdata_get_value(string{"_two dot.a.two_1a"}), string{"a_one_value_1"}));
  ASSERT_TRUE(equals(f$confdata_get_value(string{"_two dot.b.two_2b"}), string{"b_one_value_2"}));
}

TEST(confdata_functions_test, test_confdata_get_values_by_unknown_wildcard) {
  init_global_confdata_confdata();

  for (auto unknown_wildcard: {"1", "k", "_tx", "_one dot x.", "_two dot.c.", "_two dot.a.x",
                               "1*", "k*", "_tx*", "_one dot x.*", "_two dot.c.*", "_two dot.a.x*"}) {
    ASSERT_EQ(f$confdata_get_values_by_any_wildcard(string{unknown_wildcard}).count(), 0);
  }
}

TEST(confdata_functions_test, test_confdata_get_values_by_any_wildcard) {
  init_global_confdata_confdata();
  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_key_1"}), array<mixed>{
    std::make_pair(mixed{string{""}}, mixed{string{"value_1"}})
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_key"}), array<mixed>{
    std::make_pair(mixed{string{"_1"}}, mixed{string{"value_1"}}),
    std::make_pair(mixed{string{"_2"}}, mixed{string{"value_2"}})
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_one dot."}), array<mixed>{
    std::make_pair(mixed{string{"one_1"}}, mixed{string{"one_value_1"}}),
    std::make_pair(mixed{string{"one_2"}}, mixed{string{"one_value_2"}}),
    std::make_pair(mixed{3}, mixed{string{"one_value_3"}}),
    std::make_pair(mixed{string{"one_4"}}, mixed{string{"one_value_4"}})
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_two dot.a."}), array<mixed>{
    std::make_pair(mixed{string{"two_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{"two_2a"}}, mixed{string{"a_one_value_2"}}),
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_two dot."}), array<mixed>{
    std::make_pair(mixed{string{"a.two_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{"a.two_2a"}}, mixed{string{"a_one_value_2"}}),
    std::make_pair(mixed{string{"b.two_1b"}}, mixed{string{"b_one_value_1"}}),
    std::make_pair(mixed{string{"b.two_2b"}}, mixed{string{"b_one_value_2"}}),
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_two dot.a.t"}), array<mixed>{
    std::make_pair(mixed{string{"wo_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{"wo_2a"}}, mixed{string{"a_one_value_2"}}),
  }));

  ASSERT_TRUE(equals(f$confdata_get_values_by_any_wildcard(string{"_two"}), array<mixed>{
    std::make_pair(mixed{string{" dot.a.two_1a"}}, mixed{string{"a_one_value_1"}}),
    std::make_pair(mixed{string{" dot.a.two_2a"}}, mixed{string{"a_one_value_2"}}),
    std::make_pair(mixed{string{" dot.b.two_1b"}}, mixed{string{"b_one_value_1"}}),
    std::make_pair(mixed{string{" dot.b.two_2b"}}, mixed{string{"b_one_value_2"}}),
  }));
}

TEST(confdata_functions_test, test_confdata_get_values_by_bad_wildcard) {
  init_global_confdata_confdata();

  for (auto bad_wildcard: {"", "*", "**", "_two*a", "*dot", "*dot*"}) {
    ASSERT_EQ(f$confdata_get_values_by_any_wildcard(string{bad_wildcard}).count(), 0);
  }
}
