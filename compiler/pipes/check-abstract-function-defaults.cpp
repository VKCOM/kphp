// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-abstract-function-defaults.h"

#include <fmt/format.h>
#include <vector>

#include "auto/compiler/vertex/vertex-op_func_param.h"
#include "auto/compiler/vertex/vertex-types.h"
#include "common/containers/final_action.h"
#include "common/termformat/termformat.h"
#include "common/wrappers/fmt_format.h"
#include "common/wrappers/iterator_range.h"
#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/class-members.h"
#include "compiler/data/function-data.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/kphp_assert.h"
#include "compiler/pipes/calc-real-defines-values.h"
#include "compiler/stage.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"
#include "compiler/vertex-util.h"

void CheckAbstractFunctionDefaults::execute(FunctionPtr interface_function, DataStream<FunctionPtr>& os) {
  stage::set_function(interface_function);
  auto forward_function_next = vk::finally([&] { os << interface_function; });

  if (!interface_function->modifiers.is_abstract()) {
    return;
  }

  auto get_default = [](VertexPtr v) {
    auto param = v.as<op_func_param>();
    if (param->has_default_value()) {
      return param->default_value();
    }
    return VertexPtr{};
  };

  auto interface_params = interface_function->get_params();
  for (auto derived_class : interface_function->class_id->get_all_derived_classes()) {
    auto* derived_method = derived_class->members.get_instance_method(interface_function->local_name());
    if (!derived_method || derived_method->function == interface_function) {
      continue;
    }
    auto derived_params = derived_method->function->get_params();

    for (auto arg_id = interface_function->get_min_argn(); arg_id < interface_params.size(); ++arg_id) {
      auto derived_default = get_default(derived_params[arg_id]);
      auto interface_default = get_default(interface_params[arg_id]);
      kphp_error(Vertex::deep_equal(VertexUtil::unwrap_inlined_define(derived_default), VertexUtil::unwrap_inlined_define(interface_default)),
                 fmt_format("default value of interface parameter:`{}` may not differ from value of derived parameter: `{}`, in function: {}",
                            TermStringFormat::paint(VertexPtrFormatter::to_string(interface_default), TermStringFormat::green),
                            TermStringFormat::paint(VertexPtrFormatter::to_string(derived_default), TermStringFormat::green),
                            TermStringFormat::paint(derived_method->function->as_human_readable(), TermStringFormat::red)));
    }
  }
}
