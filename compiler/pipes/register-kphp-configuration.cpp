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

    // TODO сделать красиво
    const char *runtime_options_name = "DEFAULT_RUNTIME_OPTIONS";
    kphp_error(c.local_name() == runtime_options_name,
               fmt_format("Got unexpected {} constant '{}'",
                          configuration_class_name_, c.local_name()));

    auto arr = c.value.try_as<op_array>();
    kphp_error(arr, fmt_format("{}::{} must be an array",
                               configuration_class_name_, runtime_options_name));
    for (const auto &opt : arr->args()) {
      auto opt_pair = opt.try_as<op_double_arrow>();
      kphp_error(opt_pair, fmt_format("{}::{} must be an associative map",
                                      configuration_class_name_, runtime_options_name));
      const auto *opt_key = GenTree::get_constexpr_string(opt_pair->key());
      kphp_error_return(opt_key, fmt_format("{}::{} map keys must be constexpr strings",
                                            configuration_class_name_, runtime_options_name));
      const auto *opt_value = GenTree::get_constexpr_string(opt_pair->value());
      kphp_error_return(opt_value, fmt_format("{}::{} map values must be constexpr strings",
                                              configuration_class_name_, runtime_options_name));
      if (*opt_key == "--confdata-blacklist") {
        re2::RE2 compiled_pattern{*opt_value, re2::RE2::CannedOptions::Quiet};
        kphp_error_return(compiled_pattern.ok(), fmt_format("Got bad regex pattern in {}::{}['{}']: {}",
                                                            configuration_class_name_, runtime_options_name, *opt_key, compiled_pattern.error()));
        G->add_kphp_runtime_opt(*opt_key);
        G->add_kphp_runtime_opt(*opt_value);
      } else {
        kphp_error(0, fmt_format("Got unexpected option {}::{}['{}']",
                                 configuration_class_name_, runtime_options_name, *opt_key));
      }
    }
  });
}
