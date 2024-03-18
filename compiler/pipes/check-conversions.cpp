// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-conversions.h"

#include "compiler/inferring/public.h"

const std::multimap<Operation, PrimitiveType> CheckConversionsPass::forbidden_conversions = {
  {op_conv_int, tp_Class},
  {op_conv_int_l, tp_Class},
  {op_conv_float, tp_Class},
  //  {op_conv_string,   tp_Class}, // allowed for classes with __toString()
  {op_conv_string_l, tp_Class},
  {op_conv_array, tp_Class},
  {op_conv_array_l, tp_Class},
  {op_conv_object, tp_Class},
  {op_conv_mixed, tp_Class},
  {op_conv_regexp, tp_Class},

  {op_conv_int, tp_array},
  {op_conv_int_l, tp_array},
  {op_conv_float, tp_array},
  {op_conv_string, tp_array},
  {op_conv_string_l, tp_array},
  {op_conv_object, tp_array},

  {op_conv_bool, tp_tuple},
  {op_conv_int, tp_tuple},
  {op_conv_int_l, tp_tuple},
  {op_conv_float, tp_tuple},
  {op_conv_string, tp_tuple},
  {op_conv_string_l, tp_tuple},
  {op_conv_object, tp_tuple},
  {op_conv_regexp, tp_tuple},

  {op_conv_bool, tp_shape},
  {op_conv_int, tp_shape},
  {op_conv_int_l, tp_shape},
  {op_conv_float, tp_shape},
  {op_conv_string, tp_shape},
  {op_conv_string_l, tp_shape},
  {op_conv_array, tp_shape},
  {op_conv_array_l, tp_shape},
  {op_conv_object, tp_shape},
  {op_conv_mixed, tp_shape},
  {op_conv_regexp, tp_shape},
};

VertexPtr CheckConversionsPass::on_enter_vertex(VertexPtr vertex) {
  if (forbidden_conversions.count(vertex->type())) {
    auto range = forbidden_conversions.equal_range(vertex->type());
    const auto *converted_expr_type = tinf::get_type(vertex.as<meta_op_unary>()->expr());

    if (vertex->type() == op_conv_mixed) {
      converted_expr_type = converted_expr_type->get_deepest_type_of_array();
    }

    auto is_forbidden_conversion = std::any_of(range.first, range.second, [=](auto &operation_and_type) {
      // we do this so the op_conv_bool doesn't report T|false and T|null, but report T.
      bool is_conversion_of_optional = operation_and_type.first == op_conv_bool && converted_expr_type->use_optional();

      return !is_conversion_of_optional && converted_expr_type->ptype() == operation_and_type.second;
    });

    kphp_error(!is_forbidden_conversion, fmt_format("conversion from {} to {} is forbidden", converted_expr_type->as_human_readable(),
                                                    TermStringFormat::paint_green(OpInfo::str(vertex->type()))));
  }
  return vertex;
}
