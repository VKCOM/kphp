// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/sync.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/class-members.h"
#include "compiler/function-colors.h"

class RegisterKphpConfiguration final : public SyncPipeF<FunctionPtr> {
  using need_profiler = std::false_type;
  using Base = SyncPipeF<FunctionPtr>;

  void handle_KphpConfiguration_class(ClassPtr klass);

  void handle_constant_runtime_options(const ClassMemberConstant &c);
  void generic_register_simple_option(VertexPtr value, vk::string_view opt_key) const noexcept;

  void register_confdata_blacklist(VertexPtr value) const noexcept;
  void register_confdata_predefined_wildcard(VertexPtr value) const noexcept;
  void register_net_dc_mask(VertexPtr value) const noexcept;

  void handle_constant_function_palette(const ClassMemberConstant &c);
  void parse_palette(VertexPtr const_val, function_palette::Palette &palette);
  void parse_palette_ruleset(VertexAdaptor<op_array> arr, function_palette::Palette &palette);
  void parse_palette_rule(VertexAdaptor<op_double_arrow> pair, function_palette::Palette &palette, function_palette::PaletteRuleset &add_to);

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

  const vk::string_view oom_handling_memory_ratio_key_{"--oom-handling-memory-ratio"};

  const vk::string_view job_workers_shared_memory_distribution_weights_{"--job-workers-shared-memory-distribution-weights"};

  const vk::string_view thread_pool_ratio_key_{"--thread-pool-ratio"};
  const vk::string_view thread_pool_size_key_{"--thread-pool-size"};
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os) final;
  void on_finish(DataStream<FunctionPtr> &os) override;
};
