#pragma once

#include "common/mixin/not_copyable.h"

#include "compiler/common.h"
#include "compiler/utils/trie.h"

template<typename T>
struct Helper : private vk::not_copyable {
  Trie<T *> trie;
  T *on_fail;

  explicit Helper(T *on_fail);
  void add_rule(const string &rule_template, T *val_, bool need_expand = true);
  void add_simple_rule(const string &rule_template, T *val);
  T *get_default();
  T *get_help(const char *s);
};


#include "helper.hpp"
