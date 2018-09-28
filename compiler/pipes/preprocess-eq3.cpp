//
// Created by pavel on 28.09.18.
//

#include "compiler/pipes/preprocess-eq3.h"

VertexPtr PreprocessEq3Pass::on_exit_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_eq3 || root->type() == op_neq3) {
    VertexAdaptor<meta_op_binary_op> eq_op = root;
    VertexPtr a = eq_op->lhs();
    VertexPtr b = eq_op->rhs();

    if (b->type() == op_var || b->type() == op_index) {
      std::swap(a, b);
    }

    if (a->type() == op_var || a->type() == op_index) {
      VertexPtr ra = a;
      while (ra->type() == op_index) {
        ra = ra.as<op_index>()->array();
      }
      bool ok = ra->type() == op_var;
      if (ok) {
        ok &= //(b->type() != op_true && b->type() != op_false) ||
          (ra->get_string() != "connection" &&
           ra->get_string().find("MC") == string::npos);
      }

      if (ok) {
        if (b->type() == op_null) {
          VertexPtr check_cmd;
          auto isset = VertexAdaptor<op_isset>::create(a);
          if (root->type() == op_neq3) {
            check_cmd = isset;
          } else {
            auto not_isset = VertexAdaptor<op_log_not>::create(isset);
            check_cmd = not_isset;
          }
          root = check_cmd;
        } else if (b->type() == op_false ||
                   (b->type() == op_string && b->get_string() == "") ||
                   (b->type() == op_int_const && b->get_string() == "0") ||
                   (b->type() == op_float_const/* && b->str_val == "0.0"*/) ||
                   (b->type() == op_array && b->empty())) {
          VertexPtr check_cmd;

          VertexPtr isset;
          VertexPtr a_copy = a.clone();
          if (b->type() == op_true || b->type() == op_false) {
            auto is_bool = VertexAdaptor<op_func_call>::create(a_copy);
            is_bool->str_val = "is_bool";
            isset = is_bool;
          } else if (b->type() == op_string) {
            auto is_string = VertexAdaptor<op_func_call>::create(a_copy);
            is_string->str_val = "is_string";
            isset = is_string;
          } else if (b->type() == op_int_const) {
            auto is_integer = VertexAdaptor<op_func_call>::create(a_copy);
            is_integer->str_val = "is_integer";
            isset = is_integer;
          } else if (b->type() == op_float_const) {
            auto is_float = VertexAdaptor<op_func_call>::create(a_copy);
            is_float->str_val = "is_float";
            isset = is_float;
          } else if (b->type() == op_array) {
            auto is_array = VertexAdaptor<op_func_call>::create(a_copy);
            is_array->str_val = "is_array";
            isset = is_array;
          } else {
            kphp_fail();
          }


          if (root->type() == op_neq3) {
            auto not_isset = VertexAdaptor<op_log_not>::create(isset);
            auto check = VertexAdaptor<op_log_or>::create(not_isset, root);
            check_cmd = check;
          } else {
            auto check = VertexAdaptor<op_log_and>::create(isset, root);
            check_cmd = check;
          }
          root = check_cmd;
        }
      }
    }
  }

  return root;
}
