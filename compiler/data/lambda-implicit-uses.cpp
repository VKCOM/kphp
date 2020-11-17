// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/lambda-implicit-uses.h"

#include "common/algorithms/contains.h"

#include "compiler/vertex.h"
#include "compiler/gentree.h"

LambdaImplicitUses::LambdaImplicitUses(VertexAdaptor<op_func_param_list> param_list, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda)
  : uses_of_lambda_{uses_of_lambda} {
  skip_set_.reserve(param_list->params().size());
  for (auto p : param_list->params()) {
    skip_set_.insert(p.as<op_func_param>()->var()->get_string());
  }
}

void LambdaImplicitUses::find(VertexPtr root) {
  if (auto v = root.try_as<op_var>()) {
    visit_var(v);
  }

  // nested anon functions or arrow functions are converted
  // to a constructor call (op_func_call), so we don't need
  // to treat them in any special way here
  if (root->type() == op_list_ce) {
    // do not capture vars in the `list(...)` assignment LHS
    return;
  }

  for (auto &v : *root) {
    find(v);
  }
}

void LambdaImplicitUses::visit_var(VertexAdaptor<op_var> v) {
  const auto &name = v->get_string();
  if (name == "this") {
    return;
  }
  if (GenTree::is_superglobal(name)) {
    return;
  }
  if (vk::contains(name, "::")) { // skip other class (and self::*) members
    return;
  }
  if (vk::contains(skip_set_, name)) {
    return;
  }
  uses_of_lambda_->emplace_back(VertexAdaptor<op_func_param>::create(v));
  skip_set_.insert(name);
}

