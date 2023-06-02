// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-kphp-configuration.h"

#include <re2/re2.h>

#include "common/algorithms/find.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/vertex-util.h"

// We use a lot of VertexUtil::get_actual_value here because
// on CalcRealDefinesAndAssignModulitesF and InlineDefinesUsagesPass
// each define was replaced with `op_define_val` vertex with `value()` equals
// defined value. Besides, there are  These vertices will be replaced with their values
// on CollectConstVarsPass

void RegisterKphpConfiguration::execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os __attribute__ ((unused))) {
  if (function->type == FunctionData::func_class_holder && function->class_id->name == configuration_class_name_) {
    stage::set_name("Register KphpConfiguration");
    handle_KphpConfiguration_class(function->class_id);
  }
  tmp_stream << function;
}

void RegisterKphpConfiguration::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();
  Base::on_finish(os);
}


void RegisterKphpConfiguration::handle_KphpConfiguration_class(ClassPtr klass) {
  kphp_error(
    !klass->members.has_any_instance_method() &&
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
      handle_constant_function_palette(c);
    } else if (c.local_name() == runtime_options_name_) {
      handle_constant_runtime_options(c);
    } else {
      kphp_error_return(0, fmt_format("Got unexpected {} constant '{}'",
                                      configuration_class_name_, c.local_name()));
    }
  });
}

void RegisterKphpConfiguration::handle_constant_function_palette(const ClassMemberConstant &c) {
  function_palette::Palette palette;
  parse_palette(c.value, palette);
  G->set_function_palette(std::move(palette));
}

void RegisterKphpConfiguration::parse_palette(VertexPtr const_val, function_palette::Palette &palette) {
  auto arr = VertexUtil::get_actual_value(const_val).try_as<op_array>();
  kphp_error_return(arr, fmt_format("{}::{} must be a constexpr array",
                                    configuration_class_name_, function_color_palette_name_));

  for (const auto &opt : arr->args()) {
    const auto ruleset_arr = VertexUtil::get_actual_value(opt).try_as<op_array>();
    kphp_error_act(ruleset_arr, fmt_format("{}::{} must contain constexpr arrays (rulesets)",
                                           configuration_class_name_, function_color_palette_name_), continue);

    parse_palette_ruleset(ruleset_arr, palette);
  }
}

void RegisterKphpConfiguration::parse_palette_ruleset(VertexAdaptor<op_array> arr, function_palette::Palette &palette) {
  function_palette::PaletteRuleset ruleset;

  for (const auto &rule_node : arr->args()) {
    auto pair = VertexUtil::get_actual_value(rule_node).try_as<op_double_arrow>();
    kphp_error_act(pair, fmt_format("{}::{} rule must be an associative map",
                                    configuration_class_name_, function_color_palette_name_), continue);

    parse_palette_rule(pair, palette, ruleset);
  }

  palette.add_ruleset(std::move(ruleset));
}

void RegisterKphpConfiguration::parse_palette_rule(VertexAdaptor<op_double_arrow> pair, function_palette::Palette &palette, function_palette::PaletteRuleset &add_to) {
  auto parse_rule_key_colors = [this, &palette](VertexPtr key) -> std::vector<function_palette::color_t> {
    auto unwrapped_key = VertexUtil::get_actual_value(key);
    const auto *rule_string = VertexUtil::get_constexpr_string(unwrapped_key);
    auto color_names_strings = split(rule_string != nullptr ? *rule_string : "", ' ');
    kphp_error(!color_names_strings.empty(), fmt_format("{}::{} map keys must be non-empty constexpr strings",
                                                        configuration_class_name_, function_color_palette_name_));
    auto mapper = vk::make_transform_iterator_range([&palette](auto color_name) {
      return palette.register_color_name(color_name);
    }, color_names_strings.begin(), color_names_strings.end());
    return {mapper.begin(), mapper.end()};
  };

  auto parse_rule_value_error = [this](VertexPtr value) -> std::string {
    auto unwrapped_value = VertexUtil::get_actual_value(value);
    const std::string *errtext_val = VertexUtil::get_constexpr_string(unwrapped_value);
    const auto ok_val = VertexUtil::get_actual_value(unwrapped_value).try_as<op_int_const>();
    kphp_error(errtext_val || (ok_val && ok_val->get_string() == "1"), fmt_format("{}::{} map values must be constexpr strings or number 1",
                                                                                  configuration_class_name_, function_color_palette_name_));
    return errtext_val != nullptr ? *errtext_val : "";
  };

  auto colors = parse_rule_key_colors(pair->key());
  auto error = parse_rule_value_error(pair->value());
  add_to.emplace_back(function_palette::PaletteRule{std::move(colors), std::move(error)});
}

// --------------------------------------------

void RegisterKphpConfiguration::handle_constant_runtime_options(const ClassMemberConstant &c) {
  auto arr = VertexUtil::get_actual_value(c.value).try_as<op_array>();
  kphp_error_return(arr, fmt_format("{}::{} must be a constexpr array",
                                    configuration_class_name_, runtime_options_name_));
  for (const auto &opt : arr->args()) {
    auto opt_pair = VertexUtil::get_actual_value(opt).try_as<op_double_arrow>();
    kphp_error_return(opt_pair, fmt_format("{}::{} must be an associative map",
                                           configuration_class_name_, runtime_options_name_));
    const auto *opt_key = VertexUtil::get_constexpr_string(VertexUtil::get_actual_value(opt_pair->key()));
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
    } else if (vk::any_of_equal(*opt_key,
                                warmup_workers_part_key_, warmup_instance_cache_elements_part_key_, warmup_timeout_sec_key_,
                                oom_handling_memory_ratio_key_)) {
      generic_register_simple_option(opt_pair->value(), *opt_key);
    } else {
      kphp_error(0, fmt_format("Got unexpected option {}::{}['{}']",
                               configuration_class_name_, runtime_options_name_, *opt_key));
    }
  }
}

void RegisterKphpConfiguration::generic_register_simple_option(VertexPtr value, vk::string_view opt_key) const noexcept {
  const auto *opt_value  = VertexUtil::get_constexpr_string(VertexUtil::get_actual_value(value));
  kphp_error_return(opt_value, fmt_format("{}::{} must be a constexpr string",
                                          configuration_class_name_, runtime_options_name_));

  G->add_kphp_runtime_opt(static_cast<std::string>(opt_key));
  G->add_kphp_runtime_opt(*opt_value);
}

void RegisterKphpConfiguration::register_confdata_blacklist(VertexPtr value) const noexcept {
  const auto *opt_value =  VertexUtil::get_constexpr_string(VertexUtil::get_actual_value(value));
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
  auto wildcards = VertexUtil::get_actual_value(value).try_as<op_array>();
  kphp_error_return(wildcards, fmt_format("{}[{}] must be a constexpr array",
                                          runtime_options_name_, confdata_predefined_wildcard_key_));
  for (const auto &wildcard : wildcards->args()) {
    const auto *wildcard_str_value =  VertexUtil::get_constexpr_string(VertexUtil::get_actual_value(wildcard));
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
  auto dc_masks = VertexUtil::get_actual_value(value).try_as<op_array>();
  kphp_error_return(dc_masks, fmt_format("{}[{}] must be a constexpr array",
                                         runtime_options_name_, net_dc_mask_key_));
  for (const auto &dc_mask_v : dc_masks->args()) {
    const auto *index_ipv4_subnet = VertexUtil::get_constexpr_string(VertexUtil::get_actual_value(dc_mask_v));
    kphp_error_return(index_ipv4_subnet && !index_ipv4_subnet->empty(),
                      fmt_format("{}[{}][] must be non empty constexpr string",
                                 runtime_options_name_, net_dc_mask_key_));
    G->add_kphp_runtime_opt(static_cast<std::string>(net_dc_mask_key_));
    G->add_kphp_runtime_opt(*index_ipv4_subnet);
  }
}
