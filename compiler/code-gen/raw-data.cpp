#include "compiler/code-gen/raw-data.h"

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"

void compile_string_raw(const string &str, CodeGenerator &W) {
  W << "\"";
  for (int i = 0; i < (int)str.size(); i++) {
    switch (str[i]) {
      case '\r':
        W << "\\r";
        break;
      case '\n':
        W << "\\n";
        break;
      case '"':
        W << "\\\"";
        break;
      case '\\':
        W << "\\\\";
        break;
      case '\'':
        W << "\\\'";
        break;
      case 0: {
        if (str[i + 1] < '0' || str[i + 1] > '9') {
          W << "\\0";
        } else {
          W << "\\000";
        }
        break;
      }
      case '\a':
        W << "\\a";
        break;
      case '\b':
        W << "\\b";
        break;
      case '\f':
        W << "\\f";
        break;
      case '\t':
        W << "\\t";
        break;
      case '\v':
        W << "\\v";
        break;
      default:
        if ((unsigned char)str[i] < 32) {
          string tmp = "\\0";
          tmp += (char)('0' + (str[i] / 8));
          tmp += (char)('0' + (str[i] % 8));
          W << tmp;
        } else {
          W << str[i];
        }
        break;
    }
  }
  W << "\"";
}

static inline int array_len() {
  return (8 * sizeof(int)) / sizeof(double);
}


std::vector<int> compile_arrays_raw_representation(const std::vector<VarPtr> &const_raw_array_vars, CodeGenerator &W) {
  if (const_raw_array_vars.empty()) {
    return std::vector<int>();
  }

  std::vector<int> shifts;
  shifts.reserve(const_raw_array_vars.size());

  int shift = 0;

  W << "static const union { struct { int a; int b; } is; double d; } raw_arrays[] = { ";

  for (auto var_it : const_raw_array_vars) {
    VertexAdaptor<op_array> vertex = var_it->init_val.as<op_array>();

    TypeData *vertex_inner_type = vertex->tinf_node.get_type()->lookup_at(Key::any_key());


    int array_size = vertex->size();
    int array_len_in_doubles = -1;

    if (0 <= array_size && array_size <= (1 << 30) - array_len()) {
      if (vertex_inner_type->ptype() == tp_int) {
        array_len_in_doubles = array_len() + (array_size + 1) / 2;
      } else if (vertex_inner_type->ptype() == tp_float) {
        array_len_in_doubles = array_len() + array_size;
      }
    }

    if (array_len_in_doubles == -1 || !can_generate_raw_representation(vertex)) {
      shifts.push_back(-1);
      continue;
    }

    if (shift != 0) {
      W << ",";
    }

    shifts.push_back(shift);
    shift += array_len_in_doubles;

    // ref_cnt, max_key
    W << "{ .is = { .a = " << int_to_str(REF_CNT_FOR_CONST) << ", .b = " << int_to_str(array_size - 1) << "}},";

    // end_.next, end_.prev
    W << "{ .is = { .a = 0, .b = 0}},";

    // int_size, int_buf_size
    W << "{ .is = { .a = " << int_to_str(array_size) << ", .b = " << int_to_str(array_size) << "}},";

    // string_size, string_buf_size
    W << "{ .is = { .a = 0 , .b = -1 }}";

    auto args_end = vertex->args().end();
    for (auto it = vertex->args().begin(); it != args_end; ++it) {
      VertexPtr actual_vertex = GenTree::get_actual_value(*it);
      kphp_assert(vertex_inner_type->ptype() == tp_int || vertex_inner_type->ptype() == tp_float);

      if (vertex_inner_type->ptype() == tp_int) {
        W << ",{ .is = { .a = " << actual_vertex << ", .b = ";
        ++it;

        if (it != args_end) {
          actual_vertex = GenTree::get_actual_value(*it);
          W << actual_vertex << "}}";
        } else {
          W << "0}}";
          break;
        }
      } else {
        assert(vertex_inner_type->ptype() == tp_float);

        W << ", { .d =" << actual_vertex << " }";
      }
    }
  }

  W << "};\n";

  return shifts;
}

bool can_generate_raw_representation(VertexAdaptor<op_array> vertex) {
  for (auto it : vertex->args()) {
    switch (GenTree::get_actual_value(it)->type()) {
      case op_int_const:
      case op_float_const:
        continue;
      default:
        return false;
    }
  }

  return true;
}
