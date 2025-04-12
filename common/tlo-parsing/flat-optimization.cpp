// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <unordered_set>

#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"

namespace vk {
namespace tlo_parsing {
namespace {

struct InlineArg {
  const arg *argument{nullptr};
  const combinator *constructor{nullptr};

  bool empty() const {
    assert(!argument == !constructor);
    return !argument;
  }

  void clear() {
    argument = nullptr;
    constructor = nullptr;
  }
};

class ExprCloner : public const_expr_visitor {
public:
  ExprCloner(const expr_base &old_expr, const InlineArg &donor) :
    old_expr_(old_expr),
    donor_(donor) {
  }

  std::unique_ptr<type_expr_base> clone() {
    donor_.argument->type_expr->visit(*this);
    assert(new_type_child_ && !new_nat_child_);
    return std::move(new_type_child_);
  }

private:
  void apply(const type_expr &tl_type_expr) final {
    assert(!new_type_child_ && !new_nat_child_);
    auto cloned = std::make_unique<type_expr>();
    cloned->flags = tl_type_expr.flags;
    cloned->type_id = tl_type_expr.type_id;
    cloned->children.reserve(tl_type_expr.children.size());

    for (const auto &child : tl_type_expr.children) {
      child->visit(*this);
      if (new_type_child_) {
        assert(!new_nat_child_);
        cloned->children.emplace_back(std::move(new_type_child_));
      } else {
        assert(new_nat_child_);
        cloned->children.emplace_back(std::move(new_nat_child_));
      }
    }
    assert(!new_type_child_ && !new_nat_child_);
    new_type_child_ = std::move(cloned);
  }

  void apply(const type_array &tl_type_array) final {
    assert(!new_type_child_ && !new_nat_child_);
    auto cloned = std::make_unique<type_array>();
    cloned->flags = tl_type_array.flags;
    cloned->cell_len = tl_type_array.cell_len;

    tl_type_array.multiplicity->visit(*this);
    assert(!new_type_child_ && new_nat_child_);
    cloned->multiplicity = std::move(new_nat_child_);

    cloned->args.reserve(tl_type_array.args.size());
    for (const auto &tl_arg : tl_type_array.args) {
      auto cloned_arg = std::make_unique<arg>();
      cloned_arg->flags = tl_arg->flags;
      cloned_arg->idx = tl_arg->idx;
      assert(tl_arg->var_num == -1);
      cloned_arg->var_num = tl_arg->var_num;
      cloned_arg->name = tl_arg->name;
      assert(tl_arg->exist_var_bit == 0);
      cloned_arg->exist_var_bit = tl_arg->exist_var_bit;
      assert(tl_arg->exist_var_num == -1);
      cloned_arg->exist_var_num = tl_arg->exist_var_num;

      tl_arg->type_expr->visit(*this);
      assert(new_type_child_ && !new_nat_child_);
      cloned_arg->type_expr = std::move(new_type_child_);
      cloned->args.emplace_back(std::move(cloned_arg));
    }

    new_type_child_ = std::move(cloned);
  }

  void apply(const type_var &tl_type_var) final {
    assert(!new_type_child_ && !new_nat_child_);
    if (in_acceptor_) {
      new_type_child_ = std::make_unique<type_var>(tl_type_var);
    } else {
      prepare_new_child(tl_type_var.var_num);
    }
  }

  void apply(const nat_var &tl_nat_var) final {
    assert(!new_type_child_ && !new_nat_child_);
    if (in_acceptor_) {
      new_nat_child_ = std::make_unique<nat_var>(tl_nat_var);
    } else {
      prepare_new_child(tl_nat_var.var_num);
    }
  }

  void apply(const nat_const &tl_nat_const) final {
    assert(!new_type_child_ && !new_nat_child_);
    new_nat_child_ = std::make_unique<nat_const>(tl_nat_const);
  }

  void prepare_new_child(int donor_var_num) {
    const size_t template_arg_number = donor_.constructor->get_type_parameter_input_index(donor_var_num);
    const auto *replacing_type_expr = old_expr_.as<type_expr>();
    assert(replacing_type_expr);
    assert(template_arg_number < replacing_type_expr->children.size());

    in_acceptor_ = true;
    replacing_type_expr->children[template_arg_number]->visit(*this);
    in_acceptor_ = false;
    assert(!new_type_child_ != !new_nat_child_);
  }

  const expr_base &old_expr_;
  const InlineArg donor_;

  bool in_acceptor_{false};
  std::unique_ptr<type_expr_base> new_type_child_;
  std::unique_ptr<nat_expr_base> new_nat_child_;
};

class ArgFlatOptimizer : public expr_visitor {
public:
  ArgFlatOptimizer(tl_scheme &scheme) :
    scheme_(scheme),
    processed_types_(scheme.types.size() + scheme.functions.size()) {
  }

  void perform_optimization(combinator &tl_combinator) {
    assert(to_inline_.empty());
    combinator *processing_combinator = &tl_combinator;
    std::swap(processing_combinator, processing_combinator_);

    for (auto &tl_arg : processing_combinator_->args) {
      arg *processing_arg = tl_arg.get();
      std::swap(processing_arg, processing_arg_);
      tl_arg->type_expr->visit(*this);
      if (auto new_expr = make_inline_expr(*tl_arg->type_expr, "argument")) {
        tl_arg->type_expr = std::move(new_expr);
      }
      std::swap(processing_arg, processing_arg_);
      assert(processing_arg == tl_arg.get());
    }

    if (processing_combinator_->is_function()) {
      arg *processing_arg = nullptr;
      std::swap(processing_arg, processing_arg_);
      processing_combinator_->result->visit(*this);
      if (!to_inline_.empty()) {
        assert(!processing_combinator_->original_result_constructor_id);
        assert(to_inline_.constructor->type_id);
        processing_combinator_->original_result_constructor_id = to_inline_.constructor->type_id;
        processing_combinator_->result = make_inline_expr(*processing_combinator_->result, "result value");
        assert(processing_combinator_->result);
      }
      std::swap(processing_arg, processing_arg_);
      assert(!processing_arg);
    }
    std::swap(processing_combinator, processing_combinator_);
    assert(processing_combinator == &tl_combinator);
  }

  bool is_type_used(int type_id) const {
    auto it = inlined_types_.find(type_id);
    return it == inlined_types_.end() ? false : !it->second;
  }

private:
  void apply(type_expr &tl_type_expr) final {
    auto tl_type_it = scheme_.types.find(tl_type_expr.type_id);
    assert(tl_type_it != scheme_.types.end());
    type &tl_type = *tl_type_it->second;

    if (processed_types_.emplace(tl_type_expr.type_id).second) {
      for (auto &ctr : tl_type.constructors) {
        perform_optimization(*ctr);
      }
    }

    assert(to_inline_.empty());
    for (auto &child : tl_type_expr.children) {
      child->visit(*this);
      if (auto new_expr = make_inline_expr(*child, "type value")) {
        child = std::move(new_expr);
      }
    }

    prepare_type_to_inline(tl_type);
    const auto it = inlined_types_.emplace(tl_type.id, !to_inline_.empty()).first;
    // once inlined, should be inlined always, or vice versa
    assert(it->second == !to_inline_.empty());
  }

  void apply(type_array &tl_type_array) final {
    assert(to_inline_.empty());
    for (auto &tl_arg : tl_type_array.args) {
      tl_arg->type_expr->visit(*this);
      if (auto new_expr = make_inline_expr(*tl_arg->type_expr, "anonymous array argument")) {
        tl_arg->type_expr = std::move(new_expr);
      }
    }
  }

  void apply(type_var &) final {}
  void apply(nat_const &) final {}
  void apply(nat_var &) final {}

  std::unique_ptr<type_expr_base> make_inline_expr(const expr_base &expr, const char *where) {
    if (to_inline_.empty()) {
      return {};
    }

    if (processing_arg_ && !expr.is_bare()) {
      throw std::runtime_error{
        "Error on processing '" + processing_combinator_->name + "." + processing_arg_->name + "': bare expected for " + where};
    }

    // Do not forget to insert!
    assert(inlined_types_.find(to_inline_.constructor->type_id) != inlined_types_.end());
    ExprCloner cloner{expr, to_inline_};
    to_inline_.clear();
    return cloner.clone();
  }

  void prepare_type_to_inline(const type &tl_type) {
    assert(to_inline_.empty());

    static const std::unordered_set<std::string> array_types{
      "Vector", "Tuple", "Dictionary", "IntKeyDictionary", "LongKeyDictionary"
    };

    // do not perform flat optimizations for array types
    if (array_types.count(tl_type.name)) {
      return;
    }

    if (tl_type.constructors.size() != 1 ||
        tl_type.constructors.front()->name.size() != tl_type.name.size() ||
        strcasecmp(tl_type.constructors.front()->name.c_str(), tl_type.name.c_str())) {
      return;
    }

    to_inline_.constructor = tl_type.constructors.front().get();
    for (const auto &ctr_arg : to_inline_.constructor->args) {
      if (!(ctr_arg->is_optional())) {
        if (to_inline_.argument) {
          to_inline_.clear();
          return;
        }
        to_inline_.argument = ctr_arg.get();
      }
    }

    // Do not inline args with field masks
    if (!to_inline_.argument || to_inline_.argument->exist_var_num >= 0) {
      to_inline_.clear();
    }
  }

  InlineArg to_inline_;
  combinator *processing_combinator_{nullptr};
  arg *processing_arg_{nullptr};
  tl_scheme &scheme_;
  std::unordered_set<int> processed_types_;
  std::unordered_map<int, bool> inlined_types_;
};

} // namespace

void perform_flat_optimization(tl_scheme &scheme) {
  ArgFlatOptimizer flat_optimizer{scheme};
  for (auto &tl_function : scheme.functions) {
    flat_optimizer.perform_optimization(*tl_function.second);
  }

  for (auto type_it = scheme.types.begin(); type_it != scheme.types.end();) {
    if (flat_optimizer.is_type_used(type_it->first)) {
      ++type_it;
    } else {
      type_it = scheme.types.erase(type_it);
    }
  }
}

} // namespace tlo_parsing
} // namespace vk
