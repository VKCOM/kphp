#pragma once

#include <vector>

#include "common/smart_ptrs/intrusive_ptr.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/data_ptr.h"

class LambdaInterfaceGenerator {
public:
  LambdaInterfaceGenerator(std::vector<vk::intrusive_ptr<Assumption>> param_assumptions, vk::intrusive_ptr<Assumption> return_assumption) noexcept:
    param_assumptions_(std::move(param_assumptions)),
    return_assumption_(std::move(return_assumption)) {
  }

  InterfacePtr generate() noexcept;

private:
  std::string generate_name() const noexcept;
  InterfacePtr generate_interface_class(std::string name) const noexcept;
  VertexAdaptor<op_func_param_list> generate_params_for_invoke_method(InterfacePtr interface) const noexcept;
  void add_assumptions(FunctionPtr invoke_method) noexcept;

private:
  std::vector<vk::intrusive_ptr<Assumption>> param_assumptions_;
  vk::intrusive_ptr<Assumption> return_assumption_;
};
