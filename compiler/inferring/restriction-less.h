// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/multi-key.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionLess : public tinf::RestrictionBase {
private:
  std::vector<tinf::Node *> stacktrace;
  std::vector<tinf::Node *> node_path_;
  static const unsigned long max_cnt_nodes_in_path = 50;
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

  tinf::Node *actual_;
  tinf::Node *expected_;

  bool is_parent_node(tinf::Node const *node);
  bool find_call_trace_with_error_impl(tinf::Node *cur_node, const TypeData *expected);
  static bool is_less(const TypeData *given, const TypeData *expected, const MultiKey *from_at = nullptr);
  void find_call_trace_with_error(tinf::Node *cur_node, const TypeData *expected_type);

public:

  RestrictionLess(tinf::Node *a, tinf::Node *b) :
    actual_(a),
    expected_(b) {
  }

  const char *get_description() final {
    return desc.c_str();
  }

  std::string get_actual_error_message();
  std::string get_stacktrace_text();

protected:
  bool check_broken_restriction_impl() final;
};
