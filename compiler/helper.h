// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/mixin/not_copyable.h"

#include "compiler/utils/string-utils.h"
#include "compiler/utils/trie.h"

inline std::vector<std::string> expand_template(const std::string &s);

template<typename T>
struct Helper : private vk::not_copyable {
  Trie<T *> trie;
  T *on_fail;

  explicit Helper(T *on_fail) :
    on_fail(on_fail) {
    assert(on_fail != nullptr);
  }

  void add_rule(const std::string &rule_template, T *val_, bool need_expand = true) {
    if (need_expand) {
      for (auto &rule : expand_template(rule_template)) {
        trie.add(rule, val_);
      }
    } else {
      trie.add(rule_template, val_);
    }
  }

  void add_simple_rule(const std::string &rule_template, T *val) {
    add_rule(rule_template, val, false);
  }

  T *get_default() {
    return on_fail;
  }

  T *get_help(const char *s) {
    if (auto best = trie.get_deepest(s)) {
      return *best;
    }
    return nullptr;
  }
};

inline std::vector<std::string> expand_template_(vk::string_view str_template) {
  std::vector<std::string> all_possible_templates{""};

  size_t si = 0;
  size_t sn = str_template.size();

  while (si < sn) {
    std::string to_append;
    if (str_template[si] == '[') {
      si++;

      while (si < sn && str_template[si] != ']') {
        if (si + 1 < sn && str_template[si + 1] == '-') {
          assert (si + 2 < sn);

          char l = str_template[si];
          char r = str_template[si + 2];
          assert (l < r);
          for (char c = l; c <= r; c++) {
            to_append += c;
          }

          si += 3;
        } else {
          to_append += str_template[si];
          si++;
        }
      }
      assert (si < sn);
      si++;
    } else {
      to_append += str_template[si];
      si++;
    }

    auto n = all_possible_templates.size();
    for (int i = 0; i < n; ++i) {
      for (size_t j = 1; j < to_append.size(); j++) {
        all_possible_templates.push_back(all_possible_templates[i] + to_append[j]);
      }
      all_possible_templates[i] += to_append[0];
    }
  }

  return all_possible_templates;
}

inline std::vector<std::string> expand_template(const std::string &s) {
  auto template_alternatives = split_skipping_delimeters(s, "|");

  std::vector<std::string> res;
  for (auto rule_template : template_alternatives) {
    auto tmp = expand_template_(rule_template);
    std::move(tmp.begin(), tmp.end(), std::back_inserter(res));
  }

  return res;
}
