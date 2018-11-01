#include "compiler/type-inferer.h"

#include <iomanip>
#include <regex>

#include "common/termformat/termformat.h"
#include "common/wrappers/string_view.h"

#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"

void init_functions_tinf_nodes(FunctionPtr function) {
  assert (function->tinf_state == 1);
  VertexRange params = function->get_params();
  function->tinf_nodes.resize(params.size() + 1);
  for (int i = 0; i < (int)function->tinf_nodes.size(); i++) {
    function->tinf_nodes[i].param_i = i - 1;
    function->tinf_nodes[i].function_ = function;
    if (i && function->type() != FunctionData::func_extern) {
      function->tinf_nodes[i].var_ = function->param_ids[i - 1];
    }
  }
}

string colored_type_out(const TypeData *type) {
  string type_str = type_out(type);
  if (vk::string_view(type_str).starts_with("std::")) {
    type_str = type_str.substr(5);
  }
  return TermStringFormat::paint(type_str, TermStringFormat::green);
}

tinf::Node *get_tinf_node(FunctionPtr function, int id) {
  if (function->tinf_state == 0) {
    if (__sync_bool_compare_and_swap(&function->tinf_state, 0, 1)) {
      init_functions_tinf_nodes(function);
      __sync_synchronize();
      function->tinf_state = 2;
    }
  }
  while (function->tinf_state != 2) {
    __sync_synchronize();
  }

  assert (-1 <= id && id + 1 < (int)function->tinf_nodes.size());
  return &function->tinf_nodes[id + 1];
}

tinf::Node *get_tinf_node(VertexPtr vertex) {
  return &vertex->tinf_node;
}

tinf::Node *get_tinf_node(VarPtr vertex) {
  return &vertex->tinf_node;
}

const TypeData *get_type(VertexPtr vertex, tinf::TypeInferer *inferer) {
  return inferer->get_type(get_tinf_node(vertex));
}

const TypeData *get_type(VarPtr var, tinf::TypeInferer *inferer) {
  return inferer->get_type(get_tinf_node(var));
}

const TypeData *get_type(FunctionPtr function, int id, tinf::TypeInferer *inferer) {
  return inferer->get_type(get_tinf_node(function, id));
}

const TypeData *fast_get_type(VertexPtr vertex) {
  return get_tinf_node(vertex)->get_type();
}

const TypeData *fast_get_type(VarPtr var) {
  return get_tinf_node(var)->get_type();
}

const TypeData *fast_get_type(FunctionPtr function, int id) {
  return get_tinf_node(function, id)->get_type();
}

void print_why_tinf_occured_error(
  const TypeData *errored_type,
  const TypeData *because_of_type,
  PrimitiveType ptype_before_error,
  tinf::Node *node1,
  tinf::Node *node2
) {
  vector<ClassPtr> classes1, classes2;
  errored_type->get_all_class_types_inside(classes1);
  because_of_type->get_all_class_types_inside(classes2);
  ClassPtr mix_class = classes1.empty() ? ClassPtr() : classes1[0];
  ClassPtr mix_class2 = classes2.empty() ? ClassPtr() : classes2[0];
  std::string desc1 = node1->get_description();
  std::string desc2 = node2 ? node2->get_description() : "unknown";

  if (mix_class && mix_class2 && mix_class != mix_class2) {
    kphp_error(0, dl_pstr("Type Error: mix classes %s and %s: %s and %s\n",
                          mix_class->name.c_str(), mix_class2->name.c_str(),
                          desc1.c_str(), desc2.c_str()));

  } else if (mix_class || mix_class2) {
    kphp_error(0, dl_pstr("Type Error: mix class %s with non-class: %s and %s\n",
                          mix_class ? mix_class->name.c_str() : mix_class2->name.c_str(),
                          desc1.c_str(), desc2.c_str()));

  } else if (ptype_before_error == tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, dl_pstr("Type Error: inconsistent tuples %s and %s\n",
                          desc1.c_str(), desc2.c_str()));

  } else if (ptype_before_error != tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, dl_pstr("Type Error: tuples are read-only (tuple %s)\n",
                          desc1.c_str()));

  } else {
    kphp_error (0, dl_pstr("Type Error [%s] updated by [%s]\n", desc1.c_str(), desc2.c_str()));
  }
}

/*** Restrictions ***/
RestrictionIsset::RestrictionIsset(tinf::Node *a) :
  a_(a) {
  //empty
}

const char *RestrictionIsset::get_description() {
  return desc.c_str();
}

RestrictionLess::row RestrictionLess::parse_description(string const &description) {
  // Все description'ы состоят из трех колонок, разделенных между собой двумя пробелами (внутри колонки двух пробелов не должно быть)
  // Здесь происходит парсинг description'а на колонки по двум пробелам в качестве разделителя
  // Это нужно для динамичекого подсчета ширины колонок
  std::smatch matched;
  if (std::regex_match(description, matched, std::regex("(.+?)\\s\\s(.*?)(\\s\\s(.*))?"))) {
    return row(matched[1], matched[2], matched[4]);
  }
  return row("", "", description);
}

string RestrictionLess::get_actual_error_message() {
  tinf::ExprNode *as_expr_0 = nullptr;
  tinf::VarNode *as_var_0 = nullptr;
  tinf::VarNode *as_var_1 = nullptr;
  tinf::VarNode *as_var_2 = nullptr;
  if (stacktrace.size() >= 2) {
    as_expr_0 = dynamic_cast<tinf::ExprNode *>(stacktrace[0]);
    as_var_0 = dynamic_cast<tinf::VarNode *>(stacktrace[0]);
    as_var_1 = dynamic_cast<tinf::VarNode *>(stacktrace[1]);
    if (as_expr_0 && as_expr_0->get_expr()->type() == op_instance_prop &&
        as_var_1 && as_var_1->is_variable()) {
      return string("Incorrect type of the following class field: ") + TermStringFormat::add_text_attribute(as_var_1->get_var_name(), TermStringFormat::bold, false) + "\n";
    }
  }
  if (stacktrace.size() >= 3) {
    as_var_2 = dynamic_cast<tinf::VarNode *>(stacktrace[2]);
    if ((!as_var_0 || as_var_0->is_variable() || as_var_0->is_return_value_from_function()) &&
        (!as_var_1 || as_var_1->is_variable() || as_var_1->is_return_value_from_function()) &&
        as_var_2 && !as_var_2->is_variable() && !as_var_2->is_return_value_from_function()) {
      return string("Incorrect type of the ") + TermStringFormat::add_text_attribute(as_var_2->get_var_as_argument_name(), TermStringFormat::bold, false) + " at " + as_var_2->get_function_name() + "\n";
    }
  }
  if (stacktrace.size() >= 3 && as_expr_0 && as_var_1 &&
      dynamic_cast<tinf::TypeNode *>(stacktrace[2]) && dynamic_cast<tinf::TypeNode *>(stacktrace[2])->type_->ptype() == tp_var) {
    return TermStringFormat::paint("Unexpected conversion to var one of the arguments of the following function:\n", TermStringFormat::red, false) + TermStringFormat::add_text_attribute(as_expr_0->get_location_text(), TermStringFormat::bold, false) + "\n";
  }
  return "Mismatch of types\n";
}

string RestrictionLess::get_stacktrace_text() {
  // Делаем красивое форматирование stacktrace'а:
  // 1) Динамически считаем ширину его колонок, выводим выровненно
  // 2) Удаляем некоторые бесполезные дубликаты строчек
  vector<row> rows;
  for (int i = 0; i < stacktrace.size(); ++i) {
    row cur = parse_description(stacktrace[i]->get_description());
    row next = (i != int(stacktrace.size() - 1) ? parse_description(stacktrace[i + 1]->get_description()) : row());
    if (cur.col[0] == "as expression:" && next.col[0] == "as variable:" && cur.col[1] == next.col[1]) {
      i++;
    }
    rows.push_back(cur);
  }
  auto ith_row = [&](int idx) -> row { return (idx < rows.size() ? rows[idx] : row()); };
  
  // Если это ошибка несовпадения phpdoc у переменной инстанса, то первым в стектрейсе идёт ->var_name, убираем
  if (vk::string_view{rows[0].col[1]}.starts_with("->")) {
    auto as_expr_0 = dynamic_cast<tinf::ExprNode *>(stacktrace[0]);
    if (as_expr_0 && as_expr_0->get_expr()->type() == op_instance_prop) {
      rows.erase(rows.begin());
    }
  }
  // Удаление дубликатов при вызове статически отнаследованных функций (2 случая)
  // 1) Дублирование аргумента, в котором произошла ошибка:
  //  $x                                        at .../VK/A.php: VK\D :: demo (inherited from VK\A) : 20
  //  0-th arg ($x)                             at static function: VK\D :: demo (inherited from VK\A)
  //  $x                                        at .../VK/A.php: VK\D :: demo : 20
  //  0-th arg ($x)                             at static function: VK\D :: demo
  //
  // 2) Дубирование return'а:
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
      string sanitized_col_i_1 = std::regex_replace(ith_row(i).col[1], std::regex(R"([\\\(\)])"), R"(\$&)");
      if (std::regex_match(ith_row(i + 2).col[1], matched, std::regex(sanitized_col_i_1 + " (\\(inherited from .+?\\))"))) {
        rows[i].col[1] += " " + matched[1].str();
        rows.erase(rows.begin() + i + 2, rows.begin() + i + 4);
      }
    }
  }
  int width[3] = {10, 15, 30};
  for (auto &row : rows) {
    width[0] = std::max(width[0], (int)row.col[0].length());
    width[1] = std::max(width[1], (int)row.col[1].length() - TermStringFormat::get_length_without_symbols(row.col[1]));
    width[2] = std::max(width[2], (int)row.col[2].length());
  }
  stringstream ss;
  for (auto &row : rows) {
    ss << std::setw(width[1] + 3 + TermStringFormat::get_length_without_symbols(row.col[1])) << std::left << row.col[1];
    ss << std::setw(width[2]) << std::left << row.col[2] << std::endl;
  }
  return ss.str();
}

bool RestrictionLess::check_broken_restriction_impl() {
  const TypeData *actual_type = actual_->get_type();
  const TypeData *expected_type = expected_->get_type();

  if (is_less(actual_type, expected_type)) {
    find_call_trace_with_error(actual_);
    desc = TermStringFormat::add_text_attribute("\n+----------------------+\n| TYPE INFERENCE ERROR |\n+----------------------+\n", TermStringFormat::bold);
    desc += TermStringFormat::paint(get_actual_error_message(), TermStringFormat::red);
    desc += "Expected type:\t" + colored_type_out(expected_type) + "\nActual type:\t" + colored_type_out(actual_type) + "\n";
    desc += TermStringFormat::add_text_attribute("+-------------+\n| STACKTRACE: |\n+-------------+", TermStringFormat::bold);
    desc += "\n";
    desc += get_stacktrace_text();
    return true;
  }

  return false;
}


static string remove_after_tab(const string &s) {
  string ns = "";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\t') {
      break;
    }

    ns += s[i];
  }
  return ns;
}

void RestrictionIsset::find_dangerous_isset_warning(const vector<tinf::Node *> &bt, tinf::Node *node, const string &msg __attribute__((unused))) {
  stringstream ss;
  ss << "isset, !==, ===, is_array or similar function result may differ from PHP\n" <<
     " Probably, this happened because " << remove_after_tab(node->get_description()) << " of type " <<
     type_out(node->get_type()) << " can't be null in KPHP, while it can be in PHP\n" <<
     " Chain of assignments:\n";

  for (auto const n : bt) {
    ss << "  " << n->get_description() << "\n";
  }
  ss << "  " << node->get_description() << "\n";
  desc = ss.str();
}

bool RestrictionIsset::isset_is_dangerous(int isset_flags, const TypeData *tp) {
  PrimitiveType ptp = tp->ptype();
  bool res = false;
  if (isset_flags & ifi_isset) {
    return ptp != tp_var;
  }
  res |= (ptp == tp_array) &&
         (isset_flags & (ifi_is_array));
  res |= (ptp == tp_bool || tp->use_or_false()) &&
         (isset_flags & (ifi_is_bool | ifi_is_scalar));
  res |= (ptp == tp_int) &&
         (isset_flags & (ifi_is_scalar | ifi_is_numeric | ifi_is_integer | ifi_is_long));
  res |= (ptp == tp_float) &&
         (isset_flags & (ifi_is_scalar | ifi_is_numeric | ifi_is_float | ifi_is_double | ifi_is_real));
  res |= (ptp == tp_string) &&
         (isset_flags & (ifi_is_scalar | ifi_is_string));
  return res;
}

bool RestrictionIsset::find_dangerous_isset_dfs(int isset_flags, tinf::Node *node,
                                                vector<tinf::Node *> *bt) {
  if ((node->isset_was & isset_flags) == isset_flags) {
    return false;
  }
  node->isset_was |= isset_flags;

  tinf::TypeNode *type_node = dynamic_cast <tinf::TypeNode *> (node);
  if (type_node != nullptr) {
    return false;
  }

  tinf::ExprNode *expr_node = dynamic_cast <tinf::ExprNode *> (node);
  if (expr_node != nullptr) {
    VertexPtr v = expr_node->get_expr();
    if (v->type() == op_index && tinf::get_type(v.as<op_index>()->array())->ptype() != tp_tuple && isset_is_dangerous(isset_flags, node->get_type())) {
      node->isset_was = -1;
      find_dangerous_isset_warning(*bt, node, "[index]");
      return true;
    }
    if (v->type() == op_var) {
      VarPtr from_var = v.as<op_var>()->get_var_id();
      if (from_var && from_var->get_uninited_flag() && isset_is_dangerous(isset_flags, node->get_type())) {
        node->isset_was = -1;
        find_dangerous_isset_warning(*bt, node, "[uninited varialbe]");
        return true;
      }

      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, get_tinf_node(from_var), bt)) {
        return true;
      }
      bt->pop_back();
    }
    if (v->type() == op_func_call) {
      FunctionPtr func = v.as<op_func_call>()->get_func_id();
      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, get_tinf_node(func, -1), bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }

  tinf::VarNode *var_node = dynamic_cast <tinf::VarNode *> (node);
  if (var_node != nullptr) {
    VarPtr from_var = var_node->get_var();
    for (auto e : var_node->get_next()) {
      if (e->from_at->begin() != e->from_at->end()) {
        continue;
      }

      tinf::Node *to_node = e->to;

      /*** function f(&$a){}; f($b) fix ***/
      if (from_var) {
        VarPtr to_var;
        tinf::VarNode *to_var_node = dynamic_cast <tinf::VarNode *> (to_node);
        if (to_var_node != nullptr) {
          to_var = to_var_node->get_var();
        }
        if (to_var && to_var->type() == VarData::var_param_t &&
            !(to_var->holder_func == from_var->holder_func)) {
          continue;
        }
      }

      bt->push_back(node);
      if (find_dangerous_isset_dfs(isset_flags, to_node, bt)) {
        return true;
      }
      bt->pop_back();
    }
    return false;
  }
  return false;
}

bool RestrictionIsset::check_broken_restriction_impl() {
  vector<tinf::Node *> bt;
  return find_dangerous_isset_dfs(a_->isset_flags, a_, &bt);
}

void tinf::VarNode::recalc(tinf::TypeInferer *inferer) {
  VarNodeRecalc f(this, inferer);
  f.run();
}

void tinf::ExprNode::recalc(tinf::TypeInferer *inferer) {
  ExprNodeRecalc f(this, inferer);
  f.run();
}

string tinf::VarNode::get_var_name() {
  return (var_ ? var_->get_human_readable_name() : "$STRANGE_VAR");
}

string tinf::VarNode::get_function_name() {
  if (!function_) {
    if (var_ && var_->holder_func) {
      function_ = var_->holder_func;
    }
  }
  if (function_) {
    return string(function_->is_static_function() ? "static " : "") + "function: " + function_->get_human_readable_name();
  }
  if (var_->is_global_var()) {
    return "global scope";
  }
  if (var_->is_class_instance_var()) {
    return string("class ") + var_->class_id->name + " : " + int_to_str(var_->as_class_instance_field()->root->location.line);
  }
  if (var_->is_class_static_var()) {
    return string("class ") + var_->class_id->name + " : " + int_to_str(var_->as_class_static_field()->root->location.line);
  }
  return "";
}

string tinf::VarNode::get_var_as_argument_name() {
  int actual_num = (function_ && function_->is_instance_function() && !function_->is_constructor()
                    ? param_i - 1 : param_i);
  return (actual_num < 0 ? "implicit arg" : "arg #" + int_to_str(actual_num)) + " (" + get_var_name() + ")";
}

string tinf::VarNode::get_description() {
  stringstream ss;
  if (is_variable()) {
    //Вывод должен совпадать с выводом в соответсвующей ветке в get_expr_description, чтобы детектились и убирались дубликаты в стектрейсе
    ss << "as variable:" << "  " << get_var_name() << " : " << colored_type_out(tinf::get_type(var_));
  } else if (is_return_value_from_function()) {
    ss << "as expression:" << "  " << "return ...";
  } else {
    std::string var_type;
    if (var_) {
      var_type = " : " + colored_type_out(tinf::get_type(var_));
    }
    ss << "as argument:" << "  " << get_var_as_argument_name() << var_type;
  }
  ss << "  " << "at " + get_function_name();
  return ss.str();
}

string tinf::TypeNode::get_description() {
  stringstream ss;
  ss << "as type:" << "  " << type_out(type_);
  return ss.str();
}

static string get_expr_description(VertexPtr expr, bool with_type_hint = true) {
  auto print_type = [&](VertexPtr type_out_of) -> string {
    return with_type_hint ? " : " + colored_type_out(tinf::get_type(type_out_of)) : "";
  };

  switch (expr->type()) {
    case op_var:
      //Вывод должен совпадать с выводом в соответсвующей ветке в tinf::VarNode::get_description, чтобы детектились и убирались дубликаты в стектрейсе
      return "$" + expr.as<op_var>()->get_var_id()->name + print_type(expr);

    case op_func_call: {
      string function_name = expr.as<op_func_call>()->get_func_id()->get_human_readable_name();
      std::smatch matched;
      if (std::regex_match(function_name, matched, std::regex("(.+)( \\(inherited from .+?\\))"))) {
        return matched[1].str() + "(...)" + matched[2].str() + print_type(expr);
      }
      return function_name + "(...)" + print_type(expr);
    }
    case op_constructor_call:
      return "new " + expr->get_string() + "()";

    case op_instance_prop:
      return "->" + expr->get_string() + print_type(expr);

    case op_index: {
      string suff = "";
      auto orig_expr = expr;
      while (expr->type() == op_index) {
        suff += "[.]";
        expr = expr.as<op_index>()->array();
      }
      return get_expr_description(expr, false) + suff + print_type(orig_expr);
    }

    case op_int_const:
    case op_float_const:
      return expr->get_string();
    case op_string:
      return '"' + expr->get_string() + '"';

    default:
      return OpInfo::str(expr->type());
  }
}

string tinf::ExprNode::get_description() {
  stringstream ss;
  ss << "as expression:" << "  " << get_expr_description(expr_) << "  " << "at " + get_location_text();
  return ss.str();
}

string tinf::ExprNode::get_location_text() {
  string location = stage::to_str(expr_->get_location());

  //Убираем дублирование имени класса в пути до класса
  std::smatch matched;
  if (std::regex_match(location, matched, std::regex("(.+?): ((.*?) :: .*)"))) {
    string class_name = replace_characters(matched[3].str(), '\\', '/');
    if (matched[1].str().find(class_name + ".php") == matched[1].str().length() - (class_name.length() + 4)) {
      return matched[2].str();
    }
  }

  string root_path = G->env().get_base_dir();
  if (vk::string_view(location).starts_with(root_path)) {
    location = string(".../") + location.substr(root_path.length());
  }
  return location;
}

const Location &tinf::ExprNode::get_location() {
  return expr_->get_location();
}

const TypeData *NodeRecalc::new_type() {
  return new_type_;
}

bool NodeRecalc::auto_edge_flag() {
  return false;
}

void NodeRecalc::add_dependency_impl(tinf::Node *from, tinf::Node *to) {
  tinf::Edge *e = new tinf::Edge();
  e->from = from;
  e->to = to;
  e->from_at = nullptr;
  inferer_->add_edge(e);
  inferer_->add_node(e->to);
}

void NodeRecalc::add_dependency(const RValue &rvalue) {
  if (auto_edge_flag() && rvalue.node != nullptr) {
    add_dependency_impl(node_, rvalue.node);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, const RValue &rvalue) {
  if (new_type_->error_flag()) {
    return;
  }
  const TypeData *type = nullptr;
  PrimitiveType ptype = new_type_->ptype();
  if (rvalue.node != nullptr) {
    if (auto_edge_flag()) {
      add_dependency_impl(node_, rvalue.node);
    }
    __sync_synchronize();
    type = rvalue.node->get_type();
  } else if (rvalue.type != nullptr) {
    type = rvalue.type;
  } else {
    kphp_fail();
  }

  if (rvalue.key != nullptr) {
    type = type->const_read_at(*rvalue.key);
  }
  if (key == nullptr) {
    key = &MultiKey::any_key(0);
  }

  if (type->error_flag()) {
    return;
  }
  if (types_stack.empty()) {
    new_type_->set_lca_at(*key, type, !rvalue.drop_or_false);
  } else {
    types_stack.back()->set_lca_at(*key, type, !rvalue.drop_or_false);
  }

  if (unlikely(new_type_->error_flag())) {
    print_why_tinf_occured_error(new_type_, type, ptype, node_, rvalue.node);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, VertexPtr expr) {
  set_lca_at(key, as_rvalue(expr));
}

void NodeRecalc::set_lca_at(const MultiKey *key, PrimitiveType ptype) {
  set_lca_at(key, as_rvalue(ptype));
}

void NodeRecalc::set_lca(const RValue &rvalue) {
  set_lca_at(nullptr, rvalue);
}

void NodeRecalc::set_lca(PrimitiveType ptype) {
  set_lca(as_rvalue(ptype));
}

void NodeRecalc::set_lca(FunctionPtr function, int id) {
  set_lca(as_rvalue(function, id));
}

void NodeRecalc::set_lca(VertexPtr vertex, const MultiKey *key /* = nullptr*/) {
  set_lca(as_rvalue(vertex, key));
}

void NodeRecalc::set_lca(const TypeData *type, const MultiKey *key /* = nullptr*/) {
  set_lca(as_rvalue(type, key));
}

void NodeRecalc::set_lca(VarPtr var) {
  set_lca(as_rvalue(var));
}

void NodeRecalc::set_lca(ClassPtr klass) {
  TypeData *type = TypeData::get_type(tp_Class)->clone();
  type->set_class_type(klass);
  set_lca(type);
  delete type;
}


NodeRecalc::NodeRecalc(tinf::Node *node, tinf::TypeInferer *inferer) :
  new_type_(nullptr),
  node_(node),
  inferer_(inferer) {}

void NodeRecalc::on_changed() {
  __sync_synchronize();

  node_->set_type(new_type_);
  new_type_ = nullptr;

  __sync_synchronize();

  AutoLocker<Lockable *> locker(node_);
  for (auto e : node_->get_rev_next()) {
    inferer_->recalc_node(e->from);
  }
}

void NodeRecalc::run() {
  const TypeData *old_type = node_->get_type();
  new_type_ = old_type->clone();

  TypeData::upd_generation(new_type_->generation());
  TypeData::generation_t old_generation = new_type_->generation();
  TypeData::inc_generation();

  do_recalc();
  new_type_->fix_inf_array();

  if (new_type_->generation() != old_generation) {
    //fprintf(stderr, "%s %s->%s\n", node_->get_description().c_str(), type_out(node_->get_type()).c_str(), type_out(new_type_).c_str());
    on_changed();
  }

  delete new_type_;
}

void NodeRecalc::push_type() {
  types_stack.push_back(TypeData::get_type(tp_Unknown)->clone());
}

TypeData *NodeRecalc::pop_type() {
  TypeData *result = types_stack.back();
  types_stack.pop_back();
  return result;
}

VarNodeRecalc::VarNodeRecalc(tinf::VarNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc(node, inferer) {
}

void VarNodeRecalc::do_recalc() {
  //fprintf (stderr, "recalc var %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());

  if (inferer_->is_finished()) {
    kphp_error (0, dl_pstr("%s: %d\n", "", node_->recalc_cnt_));
    kphp_fail();
  }
  for (auto e : node_->get_next()) {
    set_lca_at(e->from_at, e->to);
    inferer_->add_node(e->to);
  }
}

template<PrimitiveType tp>
void ExprNodeRecalc::recalc_ptype() {

  set_lca(TypeData::get_type(tp));
}

void ExprNodeRecalc::recalc_require(VertexAdaptor<op_require> require) {
  FunctionPtr last_function = require->back()->get_func_id();
  set_lca(last_function, -1);
}

void ExprNodeRecalc::recalc_ternary(VertexAdaptor<op_ternary> ternary) {
  set_lca(ternary->true_expr());
  set_lca(ternary->false_expr());
}

void ExprNodeRecalc::apply_type_rule_func(VertexAdaptor<op_type_rule_func> func_type_rule, VertexPtr expr) {
  if (func_type_rule->str_val == "lca") {
    for (auto i : func_type_rule->args()) {
      //TODO: is it hack?
      apply_type_rule(i, expr);
    }
  } else if (func_type_rule->str_val == "OrFalse") {
    if (kphp_error (!func_type_rule->args().empty(), "OrFalse with no arguments")) {
      recalc_ptype<tp_Error>();
    } else {
      apply_type_rule(func_type_rule->args()[0], expr);
      recalc_ptype<tp_False>();
    }
  } else if (func_type_rule->str_val == "callback_call") {
    kphp_assert(func_type_rule->size() == 1);

    VertexAdaptor<op_arg_ref> arg = func_type_rule->args()[0];
    int callback_arg_id = arg->int_val;
    if (!expr || callback_arg_id < 1 || expr->type() != op_func_call || callback_arg_id > (int)expr->get_func_id()->get_params().size()) {
      kphp_error (0, "error in type rule");
      recalc_ptype<tp_Error>();
    }
    const FunctionPtr called_function = expr->get_func_id();
    kphp_assert(called_function->is_extern && called_function->get_params()[callback_arg_id - 1]->type() == op_func_param_callback);

    VertexRange call_args = expr.as<op_func_call>()->args();
    VertexPtr callback_arg = call_args[callback_arg_id - 1];

    if (callback_arg->type() == op_func_ptr) {
      set_lca(callback_arg->get_func_id(), -1);
    } else {
      recalc_ptype<tp_Error>();
    }
  } else {
    kphp_error (0, dl_pstr("unknown type_rule function [%s]", func_type_rule->str_val.c_str()));
    recalc_ptype<tp_Error>();
  }
}

void ExprNodeRecalc::apply_type_rule_type(VertexAdaptor<op_type_rule> rule, VertexPtr expr) {
  set_lca(rule->type_help);
  if (!rule->empty()) {
    push_type();
    apply_type_rule(rule->args()[0], expr);
    TypeData *tmp = pop_type();
    set_lca_at(&MultiKey::any_key(1), tmp);
    delete tmp;
  }
}

void ExprNodeRecalc::apply_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr) {
  int i = arg->int_val;
  if (!expr || i < 1 || expr->type() != op_func_call ||
      i > (int)expr->get_func_id()->get_params().size()) {
    kphp_error (0, "error in type rule");
    recalc_ptype<tp_Error>();
  }

  VertexRange call_args = expr.as<op_func_call>()->args();
  if (i - 1 < (int)call_args.size()) {
    set_lca(call_args[i - 1]);
  }
}

void ExprNodeRecalc::apply_index(VertexAdaptor<op_index> index, VertexPtr expr) {
  push_type();
  apply_type_rule(index->array(), expr);
  TypeData *type = pop_type();
  set_lca(type, &MultiKey::any_key(1));
  delete type;
}

void ExprNodeRecalc::apply_type_rule(VertexPtr rule, VertexPtr expr) {
  switch (rule->type()) {
    case op_type_rule_func:
      apply_type_rule_func(rule, expr);
      break;
    case op_type_rule:
      apply_type_rule_type(rule, expr);
      break;
    case op_arg_ref:
      apply_arg_ref(rule, expr);
      break;
    case op_index:
      apply_index(rule, expr);
      break;
    case op_class_type_rule:
      set_lca(rule.as<op_class_type_rule>()->class_ptr);
      break;
    default:
      kphp_error (0, "error in type rule");
      recalc_ptype<tp_Error>();
      break;
  }
}

void ExprNodeRecalc::recalc_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr function = call->get_func_id();
  if (call->type_rule) {
    apply_type_rule(call->type_rule.as<meta_op_type_rule>()->expr(), call);
    return;
  }

  if (function->root->type_rule) {
    apply_type_rule(function->root->type_rule.as<meta_op_type_rule>()->expr(), call);
  } else {
    set_lca(function, -1);
  }
}

void ExprNodeRecalc::recalc_constructor_call(VertexAdaptor<op_constructor_call> call) {
  FunctionPtr function = call->get_func_id();
  if (likely(static_cast<bool>(function->class_id))) {
    set_lca(function->class_id);
  } else {
    if (call->type_help == tp_MC || call->type_help == tp_Exception) {
      set_lca(call->type_help);
    } else {
      kphp_error (0, "op_constructor_call has class_id nullptr");
    }
  }
}

void ExprNodeRecalc::recalc_var(VertexAdaptor<op_var> var) {
  set_lca(var->get_var_id());
}

void ExprNodeRecalc::recalc_push_back_return(VertexAdaptor<op_push_back_return> pb) {
  set_lca(pb->array(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_index(VertexAdaptor<op_index> index) {
  bool is_const_int_index = index->has_key() && GenTree::get_actual_value(index->key())->type() == op_int_const;
  if (!is_const_int_index) {
    set_lca(index->array(), &MultiKey::any_key(1));
    return;
  }

  long int_index = parse_int_from_string(GenTree::get_actual_value(index->key()).as<op_int_const>());
  MultiKey key({Key::int_key((int)int_index)});
  set_lca(index->array(), &key);
}

void ExprNodeRecalc::recalc_instance_prop(VertexAdaptor<op_instance_prop> index) {
  set_lca(index->get_var_id());
}

void ExprNodeRecalc::recalc_set(VertexAdaptor<op_set> set) {
  set_lca(set->lhs());
}

void ExprNodeRecalc::recalc_double_arrow(VertexAdaptor<op_double_arrow> arrow) {
  set_lca(arrow->value());
}

void ExprNodeRecalc::recalc_foreach_param(VertexAdaptor<op_foreach_param> param) {
  set_lca(param->xs(), &MultiKey::any_key(1));
}

void ExprNodeRecalc::recalc_conv_array(VertexAdaptor<meta_op_unary> conv) {
  VertexPtr arg = conv->expr();
  //FIXME: (extra dependenty)
  add_dependency(as_rvalue(arg));
  if (fast_get_type(arg)->get_real_ptype() == tp_array) {
    set_lca(drop_or_false(as_rvalue(arg)));
  } else if (fast_get_type(arg)->ptype() == tp_tuple) {   // foreach/array_map/(array) на tuple'ах — ошибка
    set_lca(TypeData::get_type(tp_Error));
  } else {
    recalc_ptype<tp_array>();
    if (fast_get_type(arg)->ptype() != tp_Unknown) { //hack
      set_lca_at(&MultiKey::any_key(1), fast_get_type(arg)->get_real_ptype());
    }
  }
}

void ExprNodeRecalc::recalc_min_max(VertexAdaptor<meta_op_builtin_func> func) {
  VertexRange args = func->args();
  if (args.size() == 1) {
    set_lca(args[0], &MultiKey::any_key(1));
  } else {
    for (auto i : args) {
      set_lca(i);
    }
  }
}

void ExprNodeRecalc::recalc_array(VertexAdaptor<op_array> array) {
  recalc_ptype<tp_array>();
  for (auto i : array->args()) {
    set_lca_at(&MultiKey::any_key(1), i);
  }
}

void ExprNodeRecalc::recalc_tuple(VertexAdaptor<op_tuple> tuple) {
  recalc_ptype<tp_tuple>();
  int index = 0;
  for (auto i: tuple->args()) {
    vector<Key> i_key_index{Key::int_key(index++)};
    MultiKey key(i_key_index);
    set_lca_at(&key, i);
  }
}

void ExprNodeRecalc::recalc_plus_minus(VertexAdaptor<meta_op_unary> expr) {
  set_lca(drop_or_false(as_rvalue(expr->expr())));
  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_var>();
  }
}

void ExprNodeRecalc::recalc_inc_dec(VertexAdaptor<meta_op_unary> expr) {
  //or false ???
  set_lca(drop_or_false(as_rvalue(expr->expr())));
}

void ExprNodeRecalc::recalc_noerr(VertexAdaptor<op_noerr> expr) {
  set_lca(as_rvalue(expr->expr()));
}


void ExprNodeRecalc::recalc_arithm(VertexAdaptor<meta_op_binary> expr) {
  VertexPtr lhs = expr->lhs();
  VertexPtr rhs = expr->rhs();

  //FIXME: (extra dependency)
  add_dependency(as_rvalue(lhs));
  add_dependency(as_rvalue(rhs));

  if (fast_get_type(lhs)->ptype() == tp_bool) {
    recalc_ptype<tp_int>();
  } else {
    set_lca(drop_or_false(as_rvalue(lhs)));
  }

  if (fast_get_type(rhs)->ptype() == tp_bool) {
    recalc_ptype<tp_int>();
  } else {
    set_lca(drop_or_false(as_rvalue(rhs)));
  }

  if (new_type()->ptype() == tp_string) {
    recalc_ptype<tp_var>();
  }
}

void ExprNodeRecalc::recalc_define_val(VertexAdaptor<op_define_val> define_val) {
  //TODO: fix?
  set_lca(define_val->define_id->val);
}

void ExprNodeRecalc::recalc_expr(VertexPtr expr) {
  switch (expr->type()) {
    case op_move:
      recalc_expr(expr.as<op_move>()->expr());
      break;
    case op_require:
      recalc_require(expr);
      break;
    case op_ternary:
      recalc_ternary(expr);
      break;
    case op_func_call:
      recalc_func_call(expr);
      break;
    case op_constructor_call:
      recalc_constructor_call(expr);
      break;
    case op_common_type_rule:
    case op_gt_type_rule:
    case op_lt_type_rule:
    case op_eq_type_rule:
      apply_type_rule(expr.as<meta_op_type_rule>()->expr(), VertexPtr());
      break;
    case op_var:
      recalc_var(expr);
      break;
    case op_push_back_return:
      recalc_push_back_return(expr);
      break;
    case op_index:
      recalc_index(expr);
      break;
    case op_instance_prop:
      recalc_instance_prop(expr);
      break;
    case op_set:
      recalc_set(expr);
      break;
    case op_false:
      recalc_ptype<tp_False>();
      break;
    case op_log_or_let:
    case op_log_and_let:
    case op_log_xor_let:
    case op_log_or:
    case op_log_and:
    case op_log_not:
    case op_conv_bool:
    case op_true:
    case op_eq2:
    case op_eq3:
    case op_neq2:
    case op_neq3:
    case op_lt:
    case op_gt:
    case op_le:
    case op_ge:
    case op_isset:
    case op_exit:
      recalc_ptype<tp_bool>();
      break;

    case op_conv_string:
    case op_concat:
    case op_string_build:
    case op_string:
      recalc_ptype<tp_string>();
      break;

    case op_conv_int:
    case op_conv_int_l:
    case op_int_const:
    case op_mod:
    case op_not:
    case op_or:
    case op_and:
    case op_xor:
    case op_fork:
      recalc_ptype<tp_int>();
      break;

    case op_conv_float:
    case op_float_const:
    case op_div:
      recalc_ptype<tp_float>();
      break;

    case op_conv_uint:
      recalc_ptype<tp_UInt>();
      break;

    case op_conv_long:
      recalc_ptype<tp_Long>();
      break;

    case op_conv_ulong:
      recalc_ptype<tp_ULong>();
      break;

    case op_conv_regexp:
      recalc_ptype<tp_regexp>();
      break;

    case op_double_arrow:
      recalc_double_arrow(expr);
      break;

    case op_foreach_param:
      recalc_foreach_param(expr);
      break;

    case op_conv_array:
    case op_conv_array_l:
      recalc_conv_array(expr);
      break;

    case op_min:
    case op_max:
      recalc_min_max(expr);
      break;

    case op_array:
      recalc_array(expr);
      break;
    case op_tuple:
      recalc_tuple(expr);
      break;

    case op_conv_var:
    case op_null:
      recalc_ptype<tp_var>();
      break;

    case op_plus:
    case op_minus:
      recalc_plus_minus(expr);
      break;

    case op_prefix_inc:
    case op_prefix_dec:
    case op_postfix_inc:
    case op_postfix_dec:
      recalc_inc_dec(expr);
      break;

    case op_noerr:
      recalc_noerr(expr);
      break;

    case op_sub:
    case op_add:
    case op_mul:
    case op_shl:
    case op_shr:
      recalc_arithm(expr);
      break;

    case op_define_val:
      recalc_define_val(expr);
      break;

    default:
      recalc_ptype<tp_var>();
      break;
  }
}

bool ExprNodeRecalc::auto_edge_flag() {
  return node_->get_recalc_cnt() == 0;
}

ExprNodeRecalc::ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc(node, inferer) {
}

tinf::ExprNode *ExprNodeRecalc::get_node() {
  return (tinf::ExprNode *)node_;
}

void ExprNodeRecalc::do_recalc() {
  tinf::ExprNode *node = get_node();
  VertexPtr expr = node->get_expr();
  //fprintf (stderr, "recalc expr %d %p %s\n", get_thread_id(), node_, node_->get_description().c_str());
  stage::set_location(expr->get_location());
  recalc_expr(expr);
}

const TypeData *tinf::get_type(VertexPtr vertex) {
  return get_type(vertex, tinf::get_inferer());
}

const TypeData *tinf::get_type(VarPtr var) {
  return get_type(var, tinf::get_inferer());
}

const TypeData *tinf::get_type(FunctionPtr function, int id) {
  return get_type(function, id, tinf::get_inferer());
}

bool RestrictionLess::ComparatorByEdgePriorityRelativeToExpectedType::is_same_vars(const tinf::Node *node, VertexPtr vertex) {
  if (const tinf::VarNode *var_node = dynamic_cast<const tinf::VarNode *>(node)) {
    if (vertex->type() == op_var) {
      if (!vertex->get_var_id() && !var_node->var_) {
        return true;
      }

      return vertex->get_var_id() && var_node->var_ &&
             vertex->get_var_id()->name == var_node->var_->name;
    }
  }

  return false;
}
