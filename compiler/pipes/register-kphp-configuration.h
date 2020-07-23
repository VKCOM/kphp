#pragma once

#include "compiler/function-pass.h"

class RegisterKphpConfiguration final : public FunctionPassBase {
public:
  string get_description() final {
    return "Register KPHP Configuration";
  }

  bool check_function(FunctionPtr function) const final;
  bool user_recursion(VertexPtr) final { return true; }
  void on_start() final;

private:
  void register_confdata_blacklist(VertexPtr value) const noexcept;
  void register_confdata_predefined_wildcard(VertexPtr value) const noexcept;

  const vk::string_view configuration_class_name_{"KphpConfiguration"};
  const vk::string_view runtime_options_name_{"DEFAULT_RUNTIME_OPTIONS"};
  const vk::string_view confdata_blacklist_key_{"--confdata-blacklist"};
  const vk::string_view confdata_predefined_wildcard_{"--confdata-predefined-wildcard"};
};
