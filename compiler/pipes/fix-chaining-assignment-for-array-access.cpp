// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-chaining-assignment-for-array-access.h"

#include "auto/compiler/vertex/vertex-types.h"
#include "compiler/data/class-data.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/inferring/public.h"
#include <cassert>


// TODO maybe on_enter? 
VertexPtr FixChainingAssignmentForArrayAccessPass::on_exit_vertex(VertexPtr root) {
  if (root->type() != op_set) {
    return root;
  }

  auto set = root.as<op_set>();
  if (set->lhs()->type() != op_func_call) {
    return root;
  }

  auto func_call = set->lhs().try_as<op_func_call>();
  if (func_call->auto_inserted) {
    if (func_call->func_id->local_name() == "offsetGet") {
      auto obj_arg = func_call->args()[0];
      const auto *tpe = tinf::get_type(obj_arg); // funny
      
      assert(tpe);
      assert(tpe->get_real_ptype() == tp_Class);

      auto klass = tpe->class_type();

      const auto *method = klass->get_instance_method("offsetSet");

      if (!method) {
        kphp_error(method, fmt_format("Class {} does not implement offsetSet", klass->name).c_str());
        return root;
      }

      auto sub_val = set->rhs();


      auto key = func_call->args()[1];

      auto zzz = VertexAdaptor<op_set_with_ret>::create(key, sub_val, obj_arg);
      zzz->set_method = method->function;
      
      return zzz;
    }
  }


  return root;
}
