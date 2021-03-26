// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-kphp-configuration.h"

#include <re2/re2.h>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"

bool RegisterKphpConfiguration::check_function(FunctionPtr function) const {
  return function->type == FunctionData::func_class_holder &&
         function->class_id->name == configuration_class_name_;
}

void RegisterKphpConfiguration::on_start() {
  const auto klass = current_function->class_id;
  kphp_error(
//    !klass->members.has_any_instance_method() &&
    !klass->members.has_any_instance_var() &&
    !klass->members.has_any_static_method() &&
    !klass->members.has_any_static_var(),
    fmt_format("{} class must be empty", configuration_class_name_)
  );

  klass->members.for_each([this](const ClassMemberConstant &c) {
    if (c.local_name() == "class") {
      return;
    }

    if (c.local_name() == function_color_palette_name_) {
      handle_function_color_palette(c);
    } else if (c.local_name() == runtime_options_name_) {
      handle_runtime_options(c);
    } else {
      kphp_error_return(0, fmt_format("Got unexpected {} constant '{}'",
                                   configuration_class_name_, c.local_name()));
    }
  });
}

void RegisterKphpConfiguration::handle_function_color_palette(const ClassMemberConstant &c) {
  auto palette = parse_palette(c);
  G->set_function_palette(std::move(palette));
}

function_palette::Palette RegisterKphpConfiguration::parse_palette(const ClassMemberConstant &c) {
  const auto arr = c.value.try_as<op_array>();
  if (!arr) {
    kphp_error(arr, fmt_format("{}::{} must be a constexpr array",
                                      configuration_class_name_, function_color_palette_name_));
    return {};
  }

  palette_groups_raw groups;

  for (const auto &opt : arr->args()) {
    const auto palette_group = opt.try_as<op_array>();
    if (!palette_group) {
      kphp_error(palette_group, fmt_format("{}::{} must be a constexpr array",
                                                  configuration_class_name_, function_color_palette_name_));
      continue;
    }

    const auto group = parse_palette_group(palette_group);
    groups.push_back(std::move(group));
  }

  auto colors_num = create_palette_colors_num(groups);
  const auto palette = create_palette(groups, std::move(colors_num));
  return palette;
}

function_palette::Palette RegisterKphpConfiguration::create_palette(const palette_groups_raw &groups, function_palette::colors_num &&colors_num) {
  function_palette::Palette palette;

  for (const auto &group : groups) {
    function_palette::PaletteGroup palette_group;

    for (const auto &rule : group) {
      std::vector<function_palette::color_t> rule_colors;
      rule_colors.reserve(rule.first.size());

      for (const auto& color : rule.first) {
        rule_colors.push_back(colors_num.at(color));
      }

      function_palette::Rule palette_rule(std::move(rule_colors), rule.second);
      palette_group.add_rule(std::move(palette_rule));
    }

    palette.add_group(std::move(palette_group));
  }

  palette.set_color_nums(std::move(colors_num));
  return palette;
}

function_palette::colors_num RegisterKphpConfiguration::create_palette_colors_num(const palette_groups_raw &groups) {
  function_palette::colors_num color_nums;
  function_palette::color_t color_num = 1;

  for (const auto &group : groups) {
    for (const auto &rule : group) {
      for (const auto &color : rule.first) {
        if (color == "*") {
          color_nums.insert(std::make_pair(color, function_palette::special_colors::any));
          continue;
        }
        color_nums.insert(std::make_pair(color, color_num));
        color_num = color_num << 1;
      }
    }
  }

  return color_nums;
}

std::vector<palette_rule_raw> RegisterKphpConfiguration::parse_palette_group(const VertexAdaptor<op_array> &arr) {
  std::vector<palette_rule_raw> group;

  for (const auto &rule_node : arr->args()) {
    auto pair = rule_node.try_as<op_double_arrow>();
    if (!pair) {
      kphp_error(pair, fmt_format("{}::{} rule must be an associative map",
                                  configuration_class_name_, function_color_palette_name_));
      continue;
    }

    const auto rule = parse_palette_rule(pair);
    group.push_back(std::move(rule));
  }

  return group;
}

palette_rule_raw RegisterKphpConfiguration::parse_palette_rule(const VertexAdaptor<op_double_arrow> &pair) {
  const auto colors = parse_palette_rule_colors(pair->key());
  const auto rule_result = parse_palette_rule_result(pair->value());
  return {std::move(colors), std::move(rule_result)};
}

std::vector<std::string> RegisterKphpConfiguration::parse_palette_rule_colors(const VertexPtr &val) {
  const auto *rule_string = GenTree::get_constexpr_string(val);
  if (rule_string == nullptr) {
    kphp_error(rule_string, fmt_format("{}::{} map keys must be constexpr strings",
                                       configuration_class_name_, function_color_palette_name_));
    return {};
  }

  const auto raw_colors = split(*rule_string, ' ');
  if (raw_colors.empty()) {
    kphp_error(0, fmt_format("{}::{} rule must not be empty",
                                    configuration_class_name_, function_color_palette_name_));
  }

  return raw_colors;
}

std::string RegisterKphpConfiguration::parse_palette_rule_result(const VertexPtr &val) {
  const auto *opt_value = GenTree::get_constexpr_string(val);
  if (opt_value != nullptr) {
    return *opt_value;
  }

  const auto opt_value_int = val.try_as<op_int_const>();
  if (!opt_value_int) {
    kphp_error(0, fmt_format("{}::{} map values must be constexpr strings or number 1",
                                    configuration_class_name_, function_color_palette_name_));
    return "";
  }

  const auto opt_value_str = opt_value_int->get_string();
  if (opt_value_str != "1") {
    kphp_error(0, fmt_format("{}::{} in the palette, the result of the rule must be either the number 1 or a string with an error",
                                    configuration_class_name_, function_color_palette_name_));
    return "";
  }

  return "";
}

void RegisterKphpConfiguration::handle_runtime_options(const ClassMemberConstant &c) {
  auto arr = c.value.try_as<op_array>();
  kphp_error_return(arr, fmt_format("{}::{} must be a constexpr array",
                                    configuration_class_name_, runtime_options_name_));
  for (const auto &opt : arr->args()) {
    auto opt_pair = opt.try_as<op_double_arrow>();
    kphp_error_return(opt_pair, fmt_format("{}::{} must be an associative map",
                                           configuration_class_name_, runtime_options_name_));
    const auto *opt_key = GenTree::get_constexpr_string(opt_pair->key());
    kphp_error_return(opt_key, fmt_format("{}::{} map keys must be constexpr strings",
                                          configuration_class_name_, runtime_options_name_));
    if (*opt_key == confdata_blacklist_key_) {
      register_confdata_blacklist(opt_pair->value());
    } else if (*opt_key == confdata_predefined_wildcard_key_) {
      register_confdata_predefined_wildcard(opt_pair->value());
    } else if (*opt_key == mysql_db_name_key_) {
      register_mysql_db_name(opt_pair->value());
    } else if (*opt_key == net_dc_mask_key_) {
      register_net_dc_mask(opt_pair->value());
    }  else if (*opt_key == warmup_workers_part_key_) {
      register_warmup_workers_part(opt_pair->value());
    }  else if (*opt_key == warmup_instance_cache_elements_part_key_) {
      register_warmup_instance_cache_elements_part(opt_pair->value());
    }  else if (*opt_key == warmup_timeout_sec_key_) {
      register_warmup_timeout_sec(opt_pair->value());
    } else {
      kphp_error(0, fmt_format("Got unexpected option {}::{}['{}']",
                               configuration_class_name_, runtime_options_name_, *opt_key));
    }
  }
}

void RegisterKphpConfiguration::generic_register_simple_option(VertexPtr value, vk::string_view opt_key) const noexcept {
  const auto *opt_value = GenTree::get_constexpr_string(value);
  kphp_error_return(opt_value, fmt_format("{}::{} must be a constexpr string",
                                          configuration_class_name_, runtime_options_name_));

  G->add_kphp_runtime_opt(static_cast<std::string>(opt_key));
  G->add_kphp_runtime_opt(*opt_value);
}

void RegisterKphpConfiguration::register_confdata_blacklist(VertexPtr value) const noexcept {
  const auto *opt_value = GenTree::get_constexpr_string(value);
  kphp_error_return(opt_value, fmt_format("{}[{}] must be a constexpr string",
                                          runtime_options_name_, confdata_blacklist_key_));

  re2::RE2 compiled_pattern{*opt_value, re2::RE2::CannedOptions::Quiet};
  kphp_error_return(compiled_pattern.ok(), fmt_format("Got bad regex pattern in {}::{}['{}']: {}",
                                                      configuration_class_name_, runtime_options_name_,
                                                      confdata_blacklist_key_, compiled_pattern.error()));
  G->add_kphp_runtime_opt(static_cast<std::string>(confdata_blacklist_key_));
  G->add_kphp_runtime_opt(*opt_value);
}

void RegisterKphpConfiguration::register_confdata_predefined_wildcard(VertexPtr value) const noexcept {
  auto wildcards = value.try_as<op_array>();
  kphp_error_return(wildcards, fmt_format("{}[{}] must be a constexpr array",
                                          runtime_options_name_, confdata_predefined_wildcard_key_));
  for (const auto &wildcard : wildcards->args()) {
    const auto *wildcard_str_value = GenTree::get_constexpr_string(wildcard);
    kphp_error_return(wildcard_str_value && !wildcard_str_value->empty(),
                      fmt_format("{}[{}][] must be non empty constexpr string",
                                 runtime_options_name_, confdata_predefined_wildcard_key_));
    G->add_kphp_runtime_opt(static_cast<std::string>(confdata_predefined_wildcard_key_));
    G->add_kphp_runtime_opt(*wildcard_str_value);
  }
}

void RegisterKphpConfiguration::register_mysql_db_name(VertexPtr value) const noexcept {
  generic_register_simple_option(value, mysql_db_name_key_);
}

void RegisterKphpConfiguration::register_net_dc_mask(VertexPtr value) const noexcept {
  auto dc_masks = value.try_as<op_array>();
  kphp_error_return(dc_masks, fmt_format("{}[{}] must be a constexpr array",
                                         runtime_options_name_, net_dc_mask_key_));
  for (const auto &dc_mask_v : dc_masks->args()) {
    const auto *index_ipv4_subnet = GenTree::get_constexpr_string(dc_mask_v);
    kphp_error_return(index_ipv4_subnet && !index_ipv4_subnet->empty(),
                      fmt_format("{}[{}][] must be non empty constexpr string",
                                 runtime_options_name_, net_dc_mask_key_));
    G->add_kphp_runtime_opt(static_cast<std::string>(net_dc_mask_key_));
    G->add_kphp_runtime_opt(*index_ipv4_subnet);
  }
}

void RegisterKphpConfiguration::register_warmup_workers_part(VertexPtr value) const noexcept {
  generic_register_simple_option(value, warmup_workers_part_key_);
}

void RegisterKphpConfiguration::register_warmup_instance_cache_elements_part(VertexPtr value) const noexcept {
  generic_register_simple_option(value, warmup_instance_cache_elements_part_key_);
}

void RegisterKphpConfiguration::register_warmup_timeout_sec(VertexPtr value) const noexcept {
  generic_register_simple_option(value, warmup_timeout_sec_key_);
}
