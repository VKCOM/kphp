// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionIsset : public tinf::RestrictionBase {
  tinf::Node *a_;
  std::string desc;

  void find_dangerous_isset_warning(std::vector<tinf::Node *> *bt, tinf::Node *node, const std::string &msg);
  bool isset_is_dangerous(int isset_flags, const TypeData *tp);
  bool find_dangerous_isset_dfs(int isset_flags, tinf::Node *node, std::vector<tinf::Node *> *bt);

public:
  explicit RestrictionIsset(tinf::Node *a);

  bool is_restriction_broken() final;
  std::string get_description() final;
};
