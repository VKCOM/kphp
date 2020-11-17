// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>
#include <unordered_set>

#include "compiler/data/vertex-adaptor.h"

// LambdaImplicitUses walks the arrow function body and finds variables
// that need to be captured (by value)
class LambdaImplicitUses {
public:
  LambdaImplicitUses(VertexAdaptor<op_func_param_list> param_list, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda);

  void find(VertexPtr root);

private:
  void visit_var(VertexAdaptor<op_var> v);

  std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda_;
  std::unordered_set<std::string> skip_set_;
};
