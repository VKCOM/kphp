#include "compiler/pipes/check-conversions.h"

#include "common/termformat/termformat.h"

#include "compiler/inferring/public.h"

const std::multimap<Operation, PrimitiveType> CheckConversionsPass::forbidden_conversions = {
  {op_conv_int,      tp_Class},
  {op_conv_int_l,    tp_Class},
  {op_conv_float,    tp_Class},
  {op_conv_string,   tp_Class},
  {op_conv_string_l, tp_Class},
  {op_conv_array,    tp_Class},
  {op_conv_array_l,  tp_Class},
  {op_conv_object,   tp_Class},
  {op_conv_mixed,    tp_Class},
  {op_conv_uint,     tp_Class},
  {op_conv_long,     tp_Class},
  {op_conv_ulong,    tp_Class},
  {op_conv_regexp,   tp_Class},

  {op_conv_int,      tp_array},
  {op_conv_int_l,    tp_array},
  {op_conv_float,    tp_array},
  {op_conv_string,   tp_array},
  {op_conv_string_l, tp_array},
  {op_conv_object,   tp_array},
  {op_conv_uint,     tp_array},
  {op_conv_long,     tp_array},
  {op_conv_ulong,    tp_array},

  {op_conv_bool,     tp_tuple},
  {op_conv_int,      tp_tuple},
  {op_conv_int_l,    tp_tuple},
  {op_conv_float,    tp_tuple},
  {op_conv_string,   tp_tuple},
  {op_conv_string_l, tp_tuple},
  {op_conv_object,   tp_tuple},
  {op_conv_uint,     tp_tuple},
  {op_conv_long,     tp_tuple},
  {op_conv_ulong,    tp_tuple},

  {op_conv_bool,     tp_shape},
  {op_conv_int,      tp_shape},
  {op_conv_int_l,    tp_shape},
  {op_conv_float,    tp_shape},
  {op_conv_string,   tp_shape},
  {op_conv_string_l, tp_shape},
  {op_conv_array,    tp_shape},
  {op_conv_array_l,  tp_shape},
  {op_conv_object,   tp_shape},
  {op_conv_mixed,    tp_shape},
  {op_conv_uint,     tp_shape},
  {op_conv_long,     tp_shape},
  {op_conv_ulong,    tp_shape},
  {op_conv_regexp,   tp_shape},
};

VertexPtr CheckConversionsPass::on_enter_vertex(VertexPtr vertex) {
  if (forbidden_conversions.count(vertex->type())) {
    auto range = forbidden_conversions.equal_range(vertex->type());
    auto converted_expr_type = tinf::get_type(vertex.as<meta_op_unary>()->expr());
    if (std::count_if(range.first, range.second, [converted_expr_type](const std::pair<Operation, PrimitiveType> &r) -> bool {
      // we do this so the op_conv_bool doesn't report T|false and T|null, but report T.
      if (r.first == op_conv_bool && converted_expr_type->use_optional()) {
        return false;
      }
      return converted_expr_type->ptype() == r.second;
    })) {
      kphp_error(false, fmt_format("{} conversion of type {} is forbidden",
                                   TermStringFormat::paint_green(OpInfo::str(vertex->type())),
                                   colored_type_out(converted_expr_type)));
    }
  }
  return vertex;
}
