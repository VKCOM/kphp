// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/restriction-stacktrace-finder.h"

#include <iomanip>
#include <regex>
#include <sstream>

#include "common/termformat/termformat.h"

#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-node.h"
#include "compiler/inferring/var-node.h"
#include "compiler/vertex.h"

// A heuristic algorithm that sorts edges to optimize their order for the error printing (based on the empirical research)
struct RestrictionStacktraceFinder::ComparatorByEdgePriorityRelativeToExpectedType {
private:
  enum {
    e_default_priority = 4
  };

  const TypeData *expected;

  static bool is_same_vars(const tinf::Node *node, VertexPtr vertex);

  int get_priority(const tinf::Edge *edge) const;

public:
  explicit ComparatorByEdgePriorityRelativeToExpectedType(const TypeData *expected) :
    expected(expected) {}

  bool is_priority_less_than_default(tinf::Node *cur_node) const;

  bool operator()(const tinf::Edge *lhs, const tinf::Edge *rhs) const {
    return get_priority(lhs) < get_priority(rhs);
  }
};

bool RestrictionStacktraceFinder::ComparatorByEdgePriorityRelativeToExpectedType::is_same_vars(const tinf::Node *node, VertexPtr vertex) {
  if (auto var_node = dynamic_cast<const tinf::VarNode *>(node)) {
    if (auto var = vertex.try_as<op_var>()) {
      if (!var->var_id && !var_node->var_) {
        return true;
      }

      return var->var_id && var_node->var_ && var->var_id->name == var_node->var_->name;
    }
  }

  return false;
}

bool RestrictionStacktraceFinder::is_parent_node(tinf::Node const *node) {
  return std::find(node_path_.begin(), node_path_.end(), node) != node_path_.end();
}

bool RestrictionStacktraceFinder::find_call_trace_with_error_impl(tinf::Node *cur_node, const TypeData *expected_type) {
  static int limit_calls = 1000000;
  limit_calls--;
  if (limit_calls <= 0) {
    return true;
  }

  ComparatorByEdgePriorityRelativeToExpectedType comparator(expected_type);

  auto &node_next = cur_node->get_next();

  if (node_next.empty()) {
    return comparator.is_priority_less_than_default(cur_node);
  }

  std::vector<tinf::Edge *> ordered_edges{node_next};
  std::sort(ordered_edges.begin(), ordered_edges.end(), comparator);

  for (auto e : ordered_edges) {
    tinf::Node *from = e->from;
    tinf::Node *to = e->to;
    assert(from == cur_node);

    if (to == cur_node) {
      kphp_warning(fmt_format("loop edge: from: {} to: {}", from->get_description(), to->get_description()));
      continue;
    }
    if (is_parent_node(to)) {
      continue;
    }
    if (node_path_.size() == max_cnt_nodes_in_path) {
      return false;
    }

    node_path_.push_back(to);

    const TypeData *expected_type_in_level_of_multi_key = expected_type;
    if (e->from_at) {
      expected_type_in_level_of_multi_key = expected_type->const_read_at(*e->from_at);
    }

    if (find_call_trace_with_error_impl(to, expected_type_in_level_of_multi_key)) {
      node_path_.pop_back();
      stacktrace.push_back(to);
      return true;
    }

    node_path_.pop_back();
  }

  return false;
}


RestrictionStacktraceFinder::RestrictionStacktraceFinder(tinf::Node *cur_node, const TypeData *expected_type) {
  assert(cur_node != nullptr);

  stacktrace.reserve(max_cnt_nodes_in_path);
  node_path_.reserve(max_cnt_nodes_in_path);

  find_call_trace_with_error_impl(cur_node, expected_type);

  stacktrace.push_back(cur_node);
  std::reverse(stacktrace.begin(), stacktrace.end());
}

int RestrictionStacktraceFinder::ComparatorByEdgePriorityRelativeToExpectedType::get_priority(const tinf::Edge *edge) const {
  const tinf::Node *from_node = edge->from;
  const tinf::Node *to_node = edge->to;
  const TypeData *to_type = to_node->get_type();
  bool different_types = !is_less_or_equal_type(to_type, expected, edge->from_at);

  if (auto expr_node = dynamic_cast<const tinf::ExprNode *>(to_node)) {
    VertexPtr expr_vertex = expr_node->get_expr();

    if (auto binary_vertex = expr_vertex.try_as<meta_op_binary>()) {
      VertexPtr lhs = binary_vertex->lhs();
      VertexPtr rhs = binary_vertex->rhs();

      if (is_same_vars(from_node, lhs)) {
        to_type = tinf::get_type(rhs);
      } else if (is_same_vars(from_node, rhs)) {
        to_type = tinf::get_type(lhs);
      }
    } else {
      to_type = tinf::get_type(expr_vertex);
    }
  } else {
    if (const auto *var_node = dynamic_cast<const tinf::VarNode *>(to_node)) {
      if (var_node->is_argument_of_function()) {
        return different_types ? 1 : (e_default_priority + 1);
      }
    } else if (dynamic_cast<const tinf::TypeNode *>(to_node)) {
      return different_types ? 2 : (e_default_priority + 2);
    }
  }

  if (to_type->ptype() == tp_array && to_type->lookup_at(Key::any_key()) == nullptr) {
    return different_types ? 3 : (e_default_priority + 3);
  }

  if (!is_less_or_equal_type(to_type, expected, edge->from_at)) {
    return 0;
  }

  return e_default_priority;
}

bool RestrictionStacktraceFinder::ComparatorByEdgePriorityRelativeToExpectedType::is_priority_less_than_default(tinf::Node *cur_node) const {
  tinf::Edge edge_with_cur_node = {
    .from    = nullptr,
    .to      = cur_node,
    .from_at = nullptr
  };

  return get_priority(&edge_with_cur_node) < ComparatorByEdgePriorityRelativeToExpectedType::e_default_priority;
}

RestrictionStacktraceFinder::row RestrictionStacktraceFinder::parse_description(string const &description) {
  // all descriptions consist of the three columns delimited by two spaces (a column should never contain two consecutive spaces);
  // this method splits a description into these columns (it's needed for the column width calculation)
  std::smatch matched;
  if (std::regex_match(description, matched, std::regex("(.+?)\\s\\s(.*?)(\\s\\s(.*))?"))) {
    return row(matched[1], matched[2], matched[4]);
  }
  return row("", "", description);
}

std::string RestrictionStacktraceFinder::get_stacktrace_text() {
  // making the stack trace pretty:
  // 1) dynamically calculate the description columns width, so the output is aligned
  // 2) delete the line duplicates that are uninformative
  vector<row> rows;
  for (int i = 0; i < stacktrace.size(); ++i) {
    row cur = parse_description(stacktrace[i]->get_description());
    row next = (i != int(stacktrace.size() - 1) ? parse_description(stacktrace[i + 1]->get_description()) : row());
    if (cur.col[0] == "as expression:" && next.col[0] == "as variable:" && cur.col[1] == next.col[1]) {
      i++;
    }
    rows.push_back(cur);
  }

  remove_duplicates_from_stacktrace(rows);

  int width[3] = {10, 15, 30};
  for (auto &row : rows) {
    width[0] = std::max(width[0], (int)row.col[0].length());
    width[1] = std::max(width[1], (int)row.col[1].length() - TermStringFormat::get_length_without_symbols(row.col[1]));
    width[2] = std::max(width[2], (int)row.col[2].length());
  }

  std::stringstream ss;
  for (auto &row : rows) {
    ss << std::setw(width[1] + 3 + TermStringFormat::get_length_without_symbols(row.col[1])) << std::left << row.col[1];
    ss << std::setw(width[2]) << std::left << row.col[2] << std::endl;
  }
  return ss.str();
}

void RestrictionStacktraceFinder::remove_duplicates_from_stacktrace(vector<row> &rows) const {
  auto ith_row = [&](int idx) -> row { return (idx < rows.size() ? rows[idx] : row()); };

  // remove the phpdoc with instance types mismatch row; it's expected to start with ->var_name
  if (vk::string_view{rows[0].col[1]}.starts_with("->")) {
    auto as_expr_0 = dynamic_cast<tinf::ExprNode *>(this->stacktrace[0]);
    if (as_expr_0 && as_expr_0->get_expr()->type() == op_instance_prop) {
      rows.erase(rows.begin());
    }
  }

  // duplicates removal for the statically inherited function calls (2 cases)
  // 1) erroneous argument duplication:
  //  $x                                        at .../VK/A.php: VK\D :: demo (inherited from VK\A) : 20
  //  0-th arg ($x)                             at static function: VK\D :: demo (inherited from VK\A)
  //  $x                                        at .../VK/A.php: VK\D :: demo : 20
  //  0-th arg ($x)                             at static function: VK\D :: demo
  //
  // 2) return duplication:
  //  VK\B :: calc(...) (inherited from VK\A)   at .../dev.php: src_dev3b832f7b\u : 12
  //  return ...                                at static function: VK\B :: calc
  //  VK\B :: calc(...) (inherited from VK\A)   at .../VK/A.php: VK\B :: calc : -1
  //  return ...                                at static function: VK\B :: calc (inherited from VK\A)
  for (int i = 0; i < rows.size(); ++i) {
    if (ith_row(i).col[0] == "as expression:" && ith_row(i + 1).col[0] == "as argument:" &&
        ith_row(i + 2).col[0] == "as expression:" && ith_row(i + 3).col[0] == "as argument:" &&
        ith_row(i).col[1] == ith_row(i + 2).col[1] && ith_row(i + 1).col[1] == ith_row(i + 3).col[1] &&
        vk::string_view(ith_row(i + 1).col[2]).starts_with("at static function: ") &&
        vk::string_view(ith_row(i + 1).col[2]).starts_with(ith_row(i + 3).col[2]) &&
        ith_row(i + 1).col[2].substr(ith_row(i + 3).col[2].length(), 17) == " (inherited from ") {
      rows.erase(rows.begin() + i + 2, rows.begin() + i + 4);
    } else if (ith_row(i + 1).col[1] == "return ..." && ith_row(i + 3).col[1] == "return ..." &&
               vk::string_view(ith_row(i + 3).col[2]).starts_with("at static function: ") &&
               vk::string_view(ith_row(i + 3).col[2]).starts_with(ith_row(i + 1).col[2]) &&
               ith_row(i + 3).col[2].substr(ith_row(i + 1).col[2].length(), 17) == " (inherited from ") {
      std::smatch matched;
      std::string sanitized_col_i_1 = std::regex_replace(ith_row(i).col[1], std::regex(R"([\\\(\)\[\]])"), R"(\$&)");
      auto temp_string = ith_row(i + 2).col[1];
      if (std::regex_match(temp_string, matched, std::regex(sanitized_col_i_1 + " (\\(inherited from .+?\\))"))) {
        rows[i].col[1] += " " + matched[1].str();
        rows.erase(rows.begin() + i + 2, rows.begin() + i + 4);
      }
    }
  }
}
