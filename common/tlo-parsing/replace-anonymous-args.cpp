// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <limits>
#include <stdexcept>
#include <unordered_set>

#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"

namespace vk {
namespace tlo_parsing {
namespace {

class SchemaManager {
public:
  explicit SchemaManager(tl_scheme& scheme)
      : scheme_(scheme),
        marked_ids_(scheme_.functions.size() + scheme_.types.size()) {}

  int get_unique_magic() {
    while (scheme_.functions.count(next_magic_) || scheme_.types.count(next_magic_)) {
      assert(next_magic_ < std::numeric_limits<int>::max());
      ++next_magic_;
    }
    return next_magic_;
  }

  bool mark_magic(int magic_id) {
    return marked_ids_.emplace(magic_id).second;
  }

  tl_scheme& get_scheme() {
    return scheme_;
  }

private:
  int next_magic_{0};
  tl_scheme& scheme_;
  std::unordered_set<int> marked_ids_;
};

class AnonymousReplacer : expr_visitor {
public:
  explicit AnonymousReplacer(SchemaManager& schema_manager)
      : schema_manager_(schema_manager) {}

  void perform_replacing(combinator& tl_combinator) {
    const combinator* processing_combinator = &tl_combinator;
    std::swap(processing_combinator, processing_combinator_);

    for (auto& tl_arg : tl_combinator.args) {
      if (tl_arg->is_optional()) {
        continue;
      }

      const arg* processing_arg = tl_arg.get();
      std::swap(processing_arg_, processing_arg);
      tl_arg->type_expr->visit(*this);
      std::swap(processing_arg_, processing_arg);
      assert(processing_arg == tl_arg.get());
    }

    if (tl_combinator.is_function()) {
      tl_combinator.result->visit(*this);
    }
    std::swap(processing_combinator, processing_combinator_);
    assert(processing_combinator == &tl_combinator);
  }

private:
  static bool is_simple_replaceable_expr(const type_expr& tl_type_expr) {
    for (const auto& child : tl_type_expr.children) {
      if (const auto* child_expr = child->as<type_expr>()) {
        if (!is_simple_replaceable_expr(*child_expr)) {
          return false;
        }
      } else {
        return false;
      }
    }
    return true;
  }

  void apply(type_expr& tl_type_expr) {
    if (schema_manager_.mark_magic(tl_type_expr.type_id)) {
      auto& tl_types = schema_manager_.get_scheme().types;
      auto type_it = tl_types.find(tl_type_expr.type_id);
      assert(type_it != tl_types.end());
      for (auto& tl_combinator : type_it->second->constructors) {
        perform_replacing(*tl_combinator);
      }
    }

    for (auto& child : tl_type_expr.children) {
      assert(!child->as<type_array>());
      child->visit(*this);
    }
  }

  void apply(type_array& tl_type_array) {
    assert(processing_arg_);
    assert(processing_combinator_);

    for (auto& tl_array_arg : tl_type_array.args) {
      if (tl_array_arg->exist_var_num != -1) {
        throw std::runtime_error{"error on processing '" + processing_combinator_->name + "." + processing_arg_->name +
                                 "': " + "forbidden to use field masks for anonymous array args"};
      }
      assert(!tl_array_arg->exist_var_bit);
      tl_array_arg->type_expr->visit(*this);
    }

    if (tl_type_array.args.size() < 2) {
      return;
    }

    bool has_other = false;
    for (const auto& tl_arg : processing_combinator_->args) {
      if (!(tl_arg->is_optional()) && processing_arg_ != tl_arg.get()) {
        has_other = true;
      }
    }

    if (!has_other && processing_combinator_->is_constructor()) {
      throw std::runtime_error{"error on processing '" + processing_combinator_->name + "." + processing_arg_->name +
                               "': " + "forbidden to use alone anonymous array with several fields in constructors"};
    }

    for (const auto& tl_array_arg : tl_type_array.args) {
      if (const auto* arg_type_expr = tl_array_arg->type_expr->as<type_expr>()) {
        if (is_simple_replaceable_expr(*arg_type_expr)) {
          continue;
        }
      }
      throw std::runtime_error{"error on processing '" + processing_combinator_->name + "." + processing_arg_->name +
                               "': " + "forbidden to use complex structs in anonymous array args"};
    }

    const int anonymous_type_id = schema_manager_.get_unique_magic();
    // anonymous ctr and type have same names, who cares?
    std::string anonymous_name = processing_combinator_->name + "_arg" + std::to_string(processing_arg_->idx) + "_item";
    auto anonymous_ctr = std::make_unique<combinator>();
    anonymous_ctr->id = schema_manager_.get_unique_magic();
    anonymous_ctr->name = anonymous_name;
    anonymous_ctr->type_id = anonymous_type_id;
    anonymous_ctr->args = std::move(tl_type_array.args);
    anonymous_ctr->kind = combinator::combinator_type::CONSTRUCTOR;
    anonymous_ctr->result = std::make_unique<vk::tlo_parsing::type_expr>(anonymous_type_id);

    auto anonymous_type = std::make_unique<type>();
    anonymous_type->name = std::move(anonymous_name);
    anonymous_type->id = anonymous_type_id;
    anonymous_type->arity = 0;
    anonymous_type->constructors.emplace_back(std::move(anonymous_ctr));
    anonymous_type->constructors_num = static_cast<int>(anonymous_type->constructors.size());
    schema_manager_.get_scheme().owner_type_magics[anonymous_type->constructors[0]->id] = anonymous_type_id;
    schema_manager_.get_scheme().magics[anonymous_type->name] = anonymous_type_id;

    const bool emplaced = schema_manager_.get_scheme().types.emplace(anonymous_type_id, std::move(anonymous_type)).second;
    assert(emplaced);

    auto anonymous_arg_type_expr = std::make_unique<type_expr>(anonymous_type_id);
    anonymous_arg_type_expr->flags = FLAG_BARE;

    auto anonymous_arg = std::make_unique<arg>();
    anonymous_arg->type_expr = std::move(anonymous_arg_type_expr);
    anonymous_arg->idx = 1;
    anonymous_arg->exist_var_num = -1;
    anonymous_arg->exist_var_bit = 0;
    anonymous_arg->var_num = -1;

    tl_type_array.args.clear();
    tl_type_array.args.emplace_back(std::move(anonymous_arg));
    tl_type_array.cell_len = static_cast<int>(tl_type_array.args.size());
  }

  void apply(type_var&) {}
  void apply(nat_const&) {}
  void apply(nat_var&) {}

  SchemaManager& schema_manager_;
  const arg* processing_arg_{nullptr};
  const combinator* processing_combinator_{nullptr};
};

} // namespace

void replace_anonymous_args(tl_scheme& scheme) {
  SchemaManager magic_manager{scheme};
  AnonymousReplacer replacer{magic_manager};
  for (auto& tl_function : scheme.functions) {
    const bool marked = magic_manager.mark_magic(tl_function.first);
    assert(marked);
    replacer.perform_replacing(*tl_function.second);
  }
}

} // namespace tlo_parsing
} // namespace vk
