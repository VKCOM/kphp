#include "compiler/inferring/ifi.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/vertex.h"

is_func_id_t get_ifi_id(VertexPtr v) {
  if (v->type() == op_unset) {
    return ifi_unset;
  }
  if (v->type() == op_isset) {
    return ifi_isset;
  }
  if (v->type() == op_eq3 || v->type() == op_neq3) {
    VertexPtr b = v.as<meta_op_binary>()->rhs();
    if (b->type() == op_var || b->type() == op_index) {
      b = v.as<meta_op_binary>()->lhs();
    }

    if (b->type() == op_false) {
      return ifi_is_bool;
    }
    if (b->type() == op_string && b->get_string() == "") {
      return ifi_is_string;
    }
    if (b->type() == op_int_const && b->get_string() == "0") {
      return ifi_is_integer;
    }
    if (b->type() == op_float_const) {
      return ifi_is_float;
    }
    if (b->type() == op_array && b->empty()) {
      return ifi_is_array;
    }
  }
  if (v->type() == op_func_call) {
    const string &name = v->get_func_id()->name;
    if (name.size() > 2 && name[0] == 'i' && name[1] == 's') {
      if (name == "is_bool") {
        return ifi_is_bool;
      }
      if (name == "is_numeric") {
        return ifi_is_numeric;
      }
      if (name == "is_scalar") {
        return ifi_is_scalar;
      }
      if (name == "is_null") {
        return ifi_is_null;
      }
      if (name == "is_bool") {
        return ifi_is_bool;
      }
      if (name == "is_integer"/* || name == "is_int" */) { // disabled, because we love ads code!
        return ifi_is_integer;
      }
      if (name == "is_long") {
        return ifi_is_long;
      }
      if (name == "is_float" || name == "is_double" || name == "is_real") {
        return ifi_is_float;
      }
      if (name == "is_string") {
        return ifi_is_string;
      }
      if (name == "is_array") {
        return ifi_is_array;
      }
      if (name == "is_object") {
        return ifi_is_object;
      }
    }
  }
  return ifi_error;
}
