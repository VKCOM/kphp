// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-array-access.h"

#include "common/algorithms/contains.h"
#include "compiler/data/class-data.h"
#include "compiler/data/vertex-adaptor.h"

#include <cassert>

static VertexPtr on_unset(VertexAdaptor<op_unset> unset) {
  if (auto func_call = unset->expr().try_as<op_func_call>()) {
    if (!func_call->auto_inserted) {
      return unset;
    }
    if (vk::contains(func_call->func_id->name, "offsetGet")) {
      auto klass = func_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for unset");

      const auto *unset_method = klass->get_instance_method("offsetUnset");
      assert(unset_method && "bad method for unset");

      func_call->str_val = unset_method->global_name();
      func_call->func_id = unset_method->function;
      return func_call;
    }
  }

  return unset;
}

static VertexPtr on_isset(VertexAdaptor<op_isset> isset) {
  if (auto func_call = isset->expr().try_as<op_func_call>()) {
    if (!func_call->auto_inserted) {
      return isset;
    }
    if (vk::contains(func_call->func_id->name, "offsetGet")) {
      auto klass = func_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for isset");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      assert(isset_method && "bad method for isset");

      func_call->str_val = isset_method->global_name();
      func_call->func_id = isset_method->function;
      return func_call;
    }
  }

  return isset;
}

// important that *must not* be used with on_enter_vertex because of infinite recursion
static VertexPtr on_empty(VertexAdaptor<op_func_call> empty_call) {
  if (auto offset_get_call = empty_call->args()[0].try_as<op_func_call>()) {
    if (!offset_get_call->auto_inserted) {
      return empty_call;
    }
    if (offset_get_call->func_id && vk::contains(offset_get_call->func_id->name, "offsetGet")) {
      auto klass = offset_get_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for isset");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      assert(isset_method && "bad method for isset");

      auto exists_call = offset_get_call.clone();

      exists_call->str_val = isset_method->global_name();
      exists_call->func_id = isset_method->function;
      /*
      for empty smth like:
      op_log_or
        op_log_not
          op_func_call f$ClassName$exists
            ...
        op_func_call empty
          op_func_call f$ClassName$get
            ...
      */

      // TODO fix locations
      auto as_or = VertexAdaptor<op_log_or>::create(VertexAdaptor<op_log_not>::create(exists_call), empty_call);

      return as_or;
    }
  }

  return empty_call;
}

VertexPtr FixArrayAccessPass::on_exit_vertex(VertexPtr root) {
  if (auto unset = root.try_as<op_unset>()) {
    return on_unset(unset);
  }
  if (auto isset = root.try_as<op_isset>()) {
    return on_isset(isset);
  }
  if (auto func_call = root.try_as<op_func_call>()) {
    if (func_call->func_id->is_extern() && func_call->func_id->name == "empty") {
      return on_empty(func_call);
    }
  }

  return root;
}
