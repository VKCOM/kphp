// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionIsset : public tinf::RestrictionBase {
public:
  tinf::Node *a_;
  std::string desc;

  explicit RestrictionIsset(tinf::Node *a);
  const char *get_description() override;
  void find_dangerous_isset_warning(const std::vector<tinf::Node *> &bt, tinf::Node *node, const std::string &msg);
  bool isset_is_dangerous(int isset_flags, const TypeData *tp);
  bool find_dangerous_isset_dfs(int isset_flags, tinf::Node *node,
                                std::vector<tinf::Node *> *bt);

protected:
  bool check_broken_restriction_impl() override;
};
