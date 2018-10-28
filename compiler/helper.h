#pragma once

#include "compiler/common.h"
#include "compiler/utils/trie.h"

template<typename T>
struct Helper {
  Trie<T *> trie;
  T *on_fail;

  explicit Helper(T *on_fail);
  void add_rule(const string &rule_template, T *val_, bool need_expand = true);
  void add_simple_rule(const string &rule_template, T *val);
  T *get_default();
  T *get_help(const char *s);

private:
  DISALLOW_COPY_AND_ASSIGN (Helper);
};


#include "helper.hpp"
