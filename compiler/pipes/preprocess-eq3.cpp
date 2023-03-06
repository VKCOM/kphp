// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/preprocess-eq3.h"

#include "common/algorithms/contains.h"
#include "common/wrappers/likely.h"

VertexPtr PreprocessEq3Pass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_function && current_function->name.find("src_index") == 0) {
    printf("Pass: %s", "PreprocessEq3\n");
    root.debugPrint();
    puts("==================");
  }
  if (root->type() == op_eq3) {
    auto eq_op = root.as<meta_op_binary>();
    VertexPtr a = eq_op->lhs();
    VertexPtr b = eq_op->rhs();

    if (a->type() == op_null || b->type() == op_null) {
      root = convert_eq3_null_to_isset(root, a->type() == op_null ? b : a);
    }
  }

  return root;
}

// $a === null is !isset($a)
// $a !== null is isset($a)
// For arrays: $a[$i] !== null is isset($a[$i]); compiled to array overloading of isset
inline VertexPtr PreprocessEq3Pass::convert_eq3_null_to_isset(VertexPtr eq_op, VertexPtr not_null) {
  VertexPtr v = not_null;
  while (v->type() == op_index) {
    v = v.as<op_index>()->array();
  }
  if (vk::any_of_equal(v->type(), op_var, op_instance_prop, op_array)) {
    // it's a kludge to make "real world" PHP code compile
    // TODO: can we get rid of this?
    if (v->has_get_string() &&
        (v->get_string() == "connection" || vk::contains(v->get_string(), "MC"))) {
      return eq_op;
    }

    auto isset = VertexAdaptor<op_isset>::create(not_null).set_location(not_null->location);
    return VertexAdaptor<op_log_not>::create(isset);
  }

  return eq_op;
}
