// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cctype>
#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/server/args-functions.h"

Optional<array<mixed>> f$getopt(const string &short_options, const array<string> &long_options,
                                [[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> rest_index) noexcept {
  if (const auto &instance_st{InstanceState::get()}; instance_st.image_kind() != ImageKind::CLI) {
    return false;
  }

  enum class OptionKind : uint8_t { OptionalValue, RequiredValue, NotAcceptValue };

  const auto &cli_opts{ComponentState::get().cli_opts};
  array<mixed> options;

  // parse short options
  for (string::size_type pos = 0; pos < short_options.size(); ++pos) {
    if (!std::isalpha(short_options[pos])) {
      continue;
    }
    string option{1, short_options[pos]};

    OptionKind kind{OptionKind::NotAcceptValue};
    // check that char followed by a colon
    if (pos + 1 < short_options.size() && short_options[pos + 1] == ':') {
      kind = OptionKind::RequiredValue;
      pos++;

      // check that char followed by a two colon
      if (pos + 1 < short_options.size() && short_options[pos + 1] == ':') {
        kind = OptionKind::OptionalValue;
        pos++;
      }
    }

    if (!cli_opts.has_key(option)) {
      // option has not been set
      continue;
    }

    const mixed &value{cli_opts.get_value(option)};
    if (kind == OptionKind::OptionalValue) {
      options.set_value(std::move(option), value.empty() ? false : value);
    } else if (kind == OptionKind::RequiredValue && !value.empty()) {
      options.set_value(std::move(option), value);
    } else if (kind == OptionKind::NotAcceptValue) {
      options.set_value(std::move(option), false);
    }
  }

  // parse long options
  for (const auto &long_option : long_options) {
    const std::string_view option_view{long_option.get_value().c_str(), long_option.get_value().size()};
    uint8_t offset{};

    OptionKind kind{OptionKind::NotAcceptValue};
    if (option_view.ends_with("::")) {
      kind = OptionKind::OptionalValue;
      offset = 2;
    } else if (option_view.ends_with(":")) {
      kind = OptionKind::RequiredValue;
      offset = 1;
    }
    string option{option_view.data(), static_cast<string::size_type>(option_view.size() - offset)};

    if (!cli_opts.has_key(option)) {
      // option has not been set
      continue;
    }

    const mixed &value{cli_opts.get_value(option)};
    if (kind == OptionKind::OptionalValue) {
      options.set_value(std::move(option), value.empty() ? false : value);
    } else if (kind == OptionKind::RequiredValue && !value.empty()) {
      options.set_value(std::move(option), value);
    } else if (kind == OptionKind::NotAcceptValue) {
      options.set_value(std::move(option), false);
    }
  }

  return options;
}
