// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <set>
#include <unordered_set>

#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"
#include "common/wrappers/string_view.h"

namespace vk {
namespace tlo_parsing {
namespace {

struct CaseInsensitiveLess {
  bool operator()(vk::string_view lhs, vk::string_view rhs) const noexcept {
    const auto cmp = strncasecmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size()));
    return cmp != 0 ? cmp < 0 : lhs.size() < rhs.size();
  }
};

class TlSchemaChecker final : const_expr_visitor {
public:
  explicit TlSchemaChecker(const tl_scheme &scheme) noexcept:
    scheme_(scheme),
    ignoring_types_({"Dictionary", "IntKeyDictionary", "LongKeyDictionary", "Tuple", "Vector",
                     "String", "Int", "Long", "Double", "Float", "Bool", "False", "True"}),
    checked_types_(scheme.types.bucket_count()) {
  }

  void check_combinator(combinator &tl_combinator) {
    vk::string_view processing_combinator_name = tl_combinator.name;
    register_name_and_check_collision(processing_combinator_name);
    std::swap(processing_combinator_name_, processing_combinator_name);
    for (auto &tl_arg : tl_combinator.args) {
      if (tl_arg->name.empty()) {
        raise_exception("Got an empty name arg");
      }
      tl_arg->type_expr->visit(*this);
    }

    if (tl_combinator.is_function()) {
      tl_combinator.result->visit(*this);
    }
    std::swap(processing_combinator_name_, processing_combinator_name);
  }

private:
  void apply(const type_expr &tl_type_expr) final {
    for (const auto &child : tl_type_expr.children) {
      child->visit(*this);
    }

    auto *tl_type = scheme_.get_type_by_magic(tl_type_expr.type_id);
    assert(tl_type);
    if (ignoring_types_.count(tl_type->name)) {
      return;
    }

    if (!tl_type->constructors.empty() && tl_type->is_polymorphic() && tl_type_expr.is_bare()) {
      raise_exception("Got polymorphic type '" + tl_type->name + "' which is used as bare");
    }

    if (tl_type->name == "Maybe") {
      assert(tl_type_expr.children.size() == 1);
      const auto &child = tl_type_expr.children.back();
      if (auto *child_as_type_expr = child->as<vk::tlo_parsing::type_expr>()) {
        auto *child_type = scheme_.get_type_by_magic(child_as_type_expr->type_id);
        assert(child_type);
        if (child_type->name == "True") {
          raise_exception("Got 'Maybe True' usage");
        }
      }
    }

    if (checked_types_.emplace(tl_type_expr.type_id).second) {
      for (const auto &tl_constructor : tl_type->constructors) {
        check_combinator(*tl_constructor);
        check_constructor(*tl_constructor);
      }
      if (tl_type->constructors.size() > 1) {
        register_name_and_check_collision(tl_type->name);
      }
    }
  }

  void apply(const type_array &tl_type_array) final {
    for (const auto &anonymous_arg : tl_type_array.args) {
      anonymous_arg->type_expr->visit(*this);
    }
  }

  void apply(const type_var &) final {}
  void apply(const nat_const &) final {}
  void apply(const nat_var &) final {}

  void check_constructor(const vk::tlo_parsing::combinator &c) {
    // Проверяем, что порядок неявных аргументов конструктора совпадает с их порядком в типе
    std::vector<int> var_nums;
    for (const auto &arg : c.args) {
      if (arg->is_optional() && arg->var_num != -1) {
        var_nums.push_back(arg->var_num);
      }
    }
    std::vector<int> params_order;
    auto *as_type_expr = c.result->as<vk::tlo_parsing::type_expr>();
    for (const auto &child : as_type_expr->children) {
      if (auto *as_nat_var = child->as<vk::tlo_parsing::nat_var>()) {
        params_order.push_back(as_nat_var->var_num);
      } else if (auto *as_type_var = child->as<vk::tlo_parsing::type_var>()) {
        params_order.push_back(as_type_var->var_num);
      }
    }
    if (var_nums != params_order) {
      raise_exception("Got implicit args order mismatching");
    }
    // Также проверяем, что индексы аргументов типовых переменных в конструкторе
    // совпадают с порядковым (слева направо, начиная с 0)
    // номером соответстсвующей вершины-ребенка в типовом дереве
    std::vector<int> indices_in_constructor;
    for (int i = 0; i < c.args.size(); ++i) {
      if (c.args[i]->is_type()) {
        indices_in_constructor.push_back(i);
      }
    }
    std::vector<int> indices_in_type_tree;
    int i = 0;
    for (const auto &child : as_type_expr->children) {
      if (dynamic_cast<vk::tlo_parsing::type_var *>(child.get())) {
        indices_in_type_tree.push_back(i);
      }
      ++i;
    }
    if (indices_in_constructor != indices_in_type_tree) {
      raise_exception("Got different indexes in constructor and child tree");
    }
  }

  void register_name_and_check_collision(vk::string_view name) {
    // для RpcReqResult не генерируются классы, поэтому тут забиваем на коллизии
    if (!used_names_.emplace(name).second && name != "RpcReqResult") {
      raise_exception("Got name '" + static_cast<std::string>(name) + "' collision");
    }
  }

  void raise_exception(const std::string &msg) {
    throw std::runtime_error{msg + " in '" + static_cast<std::string>(processing_combinator_name_) + "'"};
  }

  const tl_scheme &scheme_;
  const std::unordered_set<vk::string_view> ignoring_types_;

  vk::string_view processing_combinator_name_;
  std::unordered_set<int> checked_types_;

  std::set<vk::string_view, CaseInsensitiveLess> used_names_;
};

} // namespace

void tl_scheme_final_check(const tl_scheme &scheme) {
  TlSchemaChecker schema_checker{scheme};
  for (const auto &tl_function : scheme.functions) {
    schema_checker.check_combinator(*tl_function.second);
  }
}

} // namespace tlo_parsing
} // namespace vk
