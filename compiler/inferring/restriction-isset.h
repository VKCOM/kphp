#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionIsset : public tinf::Restriction {
public:
  tinf::Node *a_;
  string desc;

  explicit RestrictionIsset(tinf::Node *a);
  const char *get_description() override;
  void find_dangerous_isset_warning(const vector<tinf::Node *> &bt, tinf::Node *node, const string &msg);
  bool isset_is_dangerous(int isset_flags, const TypeData *tp);
  bool find_dangerous_isset_dfs(int isset_flags, tinf::Node *node,
                                vector<tinf::Node *> *bt);

protected:
  bool check_broken_restriction_impl() override;
};
