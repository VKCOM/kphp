#include "compiler/code-gen/raw-data.h"

#include <sstream>

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/const-manipulations.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"

void RawString::compile(CodeGenerator &W) const {
  W << "\"";
  for (size_t i = 0; i < str.size(); i++) {
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
  return (10 * sizeof(int)) / sizeof(double);
}


std::vector<int> compile_arrays_raw_representation(const std::vector<VarPtr> &const_raw_array_vars, CodeGenerator &W) {
  if (const_raw_array_vars.empty()) {
    return std::vector<int>();
  }

  std::vector<int> shifts;
  shifts.reserve(const_raw_array_vars.size());

  int shift = 0;

  for (auto var_it : const_raw_array_vars) {
    VertexAdaptor<op_array> vertex = var_it->init_val.as<op_array>();

    TypeData *vertex_inner_type = vertex->tinf_node.get_type()->lookup_at(Key::any_key());


    int array_size = vertex->size();
    int array_len_in_doubles = -1;

    if (0 <= array_size && array_size <= (1 << 30) - array_len()) {
      if (vk::any_of_equal(vertex_inner_type->ptype(), tp_int, tp_float)) {
        array_len_in_doubles = array_len() + array_size;
      }
    }

    if (array_len_in_doubles == -1 || !CanGenerateRawArray::is_raw(vertex)) {
      shifts.push_back(-1);
      continue;
    }

    if (shift != 0) {
      W << ",";
    } else {
      W << "static_assert(sizeof(array<Unknown>::iterator::inner_type) == " << array_len() * sizeof(double) << ", \"size of array_len should be compatible with runtime array_inner\");" << NL;
      W << "static const union " << BEGIN
        << "struct { uint32_t a; uint32_t b; } is;" << NL
        << "double d;" << NL
        << "int64_t i64;" << NL
        << END
        << " raw_arrays[] = { ";
    }

    shifts.push_back(shift);
    shift += array_len_in_doubles;

    // stub, ref_cnt
    W << "{ .is = { .a = 0, .b = " << ExtraRefCnt::for_global_const << "}},";
    // max_key
    W << "{ .i64 = " << array_size - 1 << "},";
    // end_.next, end_.prev
    W << "{ .is = { .a = 0, .b = 0}},";

    // int_size, int_buf_size
    W << "{ .is = { .a = " << array_size << ", .b = " << array_size << "}},";

    // string_size, string_buf_size
    W << "{ .is = { .a = 0 , .b = " << std::numeric_limits<uint32_t>::max() << " }}";

    auto args_end = vertex->args().end();
    for (auto it = vertex->args().begin(); it != args_end; ++it) {
      VertexPtr actual_vertex = GenTree::get_actual_value(*it);
      if (auto double_arrow = actual_vertex.try_as<op_double_arrow>()) {
        actual_vertex = GenTree::get_actual_value(double_arrow->value());
      }
      kphp_assert(vk::any_of_equal(vertex_inner_type->ptype(), tp_int, tp_float));

      if (vertex_inner_type->ptype() == tp_int) {
        W << ", { .i64 =" << actual_vertex << " }";
      } else {
        W << ", { .d =" << actual_vertex << " }";
      }
    }
  }

  if (shift) {
    W << "};\n";
  }

  return shifts;
}

