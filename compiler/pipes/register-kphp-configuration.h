// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

using palette_rule_raw = std::pair<std::vector<std::string>, std::string>;
using palette_group_raw = std::vector<palette_rule_raw>;
using palette_groups_raw = std::vector<palette_group_raw>;

class RegisterKphpConfiguration final : public FunctionPassBase {
public:
  string get_description() final {
    return "Register KPHP Configuration";
  }

  bool check_function(FunctionPtr function) const final;
  bool user_recursion(VertexPtr) final { return true; }
  void on_start() final;

private:
  void handle_runtime_options(const ClassMemberConstant &c);

  void handle_function_color_palette(const ClassMemberConstant &c);
  function_palette::Palette parse_palette(const ClassMemberConstant &c);
  function_palette::Palette create_palette(const palette_groups_raw &groups, function_palette::colors_num &&colors_num);
  function_palette::colors_num create_palette_colors_num(const palette_groups_raw &groups);
  std::vector<palette_rule_raw> parse_palette_group(const VertexAdaptor<op_array> &arr);
  palette_rule_raw parse_palette_rule(const VertexAdaptor<op_double_arrow> &pair);
  std::string parse_palette_rule_result(const VertexPtr &pair);
  std::vector<std::string> parse_palette_rule_colors(const VertexPtr &pair);

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
  const vk::string_view function_color_palette_name_{"FUNCTION_PALETTE"};

  const vk::string_view confdata_blacklist_key_{"--confdata-blacklist"};
  const vk::string_view confdata_predefined_wildcard_key_{"--confdata-predefined-wildcard"};
  const vk::string_view mysql_db_name_key_{"--mysql-db-name"};
  const vk::string_view net_dc_mask_key_{"--net-dc-mask"};

  const vk::string_view warmup_workers_part_key_{"--warmup-workers-ratio"};
  const vk::string_view warmup_instance_cache_elements_part_key_{"--warmup-instance-cache-elements-ratio"};
  const vk::string_view warmup_timeout_sec_key_{"--warmup-timeout"};
};
