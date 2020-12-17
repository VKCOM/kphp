// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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
  void generic_register_simple_option(VertexPtr value, vk::string_view opt_key) const noexcept;

  void register_confdata_blacklist(VertexPtr value) const noexcept;
  void register_confdata_predefined_wildcard(VertexPtr value) const noexcept;
  void register_mysql_db_name(VertexPtr value) const noexcept;
  void register_net_dc_mask(VertexPtr value) const noexcept;
  void register_warmup_workers_part(VertexPtr value) const noexcept;
  void register_warmup_instance_cache_elements_part(VertexPtr value) const noexcept;
  void register_warmup_timeout_sec(VertexPtr value) const noexcept;


  const vk::string_view configuration_class_name_{"KphpConfiguration"};
  const vk::string_view runtime_options_name_{"DEFAULT_RUNTIME_OPTIONS"};
  
  const vk::string_view confdata_blacklist_key_{"--confdata-blacklist"};
  const vk::string_view confdata_predefined_wildcard_key_{"--confdata-predefined-wildcard"};
  const vk::string_view mysql_db_name_key_{"--mysql-db-name"};
  const vk::string_view net_dc_mask_key_{"--net-dc-mask"};

  const vk::string_view warmup_workers_part_key_{"--warmup-workers-part"};
  const vk::string_view warmup_instance_cache_elements_part_key_{"--warmup-instance-cache-elements-part"};
  const vk::string_view warmup_timeout_sec_key_{"--warmup-timeout-sec"};
};
