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
  std::vector<tinf::Node *> node_path_;
  std::string desc;

  struct ComparatorByEdgePriorityRelativeToExpectedType;

  struct row {
    std::string col[3];

    row() = default;

    row(std::string const &s1, std::string const &s2, std::string const &s3) {
      col[0] = s1;
      col[1] = s2;
      col[2] = s3;
    }
  };

  row parse_description(std::string const &description);
  void remove_duplicates_from_stacktrace(std::vector<row> &rows) const;

  bool is_parent_node(tinf::Node const *node);
  bool find_call_trace_with_error_impl(tinf::Node *cur_node, const TypeData *expected_type);

public:

  RestrictionStacktraceFinder(tinf::Node *cur_node, const TypeData *expected_type);

  std::string get_stacktrace_text();
};
