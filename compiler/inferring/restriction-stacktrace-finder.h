// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/multi-key.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionStacktraceFinder {
  static const unsigned long max_cnt_nodes_in_path = 50;

  std::vector<tinf::Node *> stacktrace;
  std::vector<tinf::Node *> node_path;
  std::string desc;

  static VarPtr get_var_id_from_node(const tinf::Node *node);
  static int get_importance_of_reason(const tinf::Node *from, const tinf::Node *to);
  static int get_priority(const tinf::Edge *edge, const TypeData *expected_type);

  bool find_call_trace_with_error(tinf::Node *cur_node, const TypeData *expected_type);

public:

  RestrictionStacktraceFinder(tinf::Node *cur_node, const TypeData *expected_type);

  std::string get_stacktrace_text();
};
