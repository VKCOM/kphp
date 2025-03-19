// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cctype>
#include <cstdint>
#include <functional>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/server/args-functions.h"

Optional<array<mixed>> f$getopt(const string &short_options, const array<string> &long_options,
                                [[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> rest_index) noexcept {
  if (const auto &instance_st{InstanceState::get()}; instance_st.image_kind() != ImageKind::CLI) [[unlikely]] {
    return false;
  }

  enum class option_kind : uint8_t { flag, required, optional };

  const auto &cli_opts{ComponentState::get().cli_opts};
  array<mixed> options;

  std::string_view short_options_view{short_options.c_str(), short_options.size()};
  // parse short options
  for (size_t pos = 0; pos < short_options_view.size(); ++pos) {
    if (!std::isalnum(short_options_view[pos])) {
      continue;
    }
    const string option{1, short_options_view[pos]};

    option_kind kind{option_kind::flag};
    // check that char followed by a colon
    if (pos + 1 < short_options_view.size() && short_options_view[pos + 1] == ':') {
      kind = option_kind::required;
      pos++;

      // check that char followed by a two colon
      if (pos + 1 < short_options_view.size() && short_options_view[pos + 1] == ':') {
        kind = option_kind::optional;
        pos++;
      }
    }

    if (!cli_opts.has_key(option)) [[unlikely]] {
      // option has not been set
      continue;
    }

    const mixed &value{cli_opts.get_value(option)};
    if (kind == option_kind::optional) {
      options.set_value(option, value.empty() ? false : value);
    } else if (kind == option_kind::required && !value.empty()) {
      options.set_value(option, value);
    } else if (kind == option_kind::flag) {
      options.set_value(option, false);
    }
  }

  // parse long options
  for (const auto &long_option : long_options) {
    const std::string_view option_view{long_option.get_value().c_str(), long_option.get_value().size()};
    uint8_t offset{};

    option_kind kind{option_kind::flag};
    if (option_view.ends_with("::")) {
      kind = option_kind::optional;
      offset = 2;
    } else if (option_view.ends_with(":")) {
      kind = option_kind::required;
      offset = 1;
    }
    const string option{option_view.data(), static_cast<string::size_type>(option_view.size() - offset)};

    if (!cli_opts.has_key(option)) [[unlikely]] {
      // option has not been set
      continue;
    }

    const mixed &value{cli_opts.get_value(option)};
    if (kind == option_kind::optional) {
      options.set_value(option, value.empty() ? false : value);
    } else if (kind == option_kind::required && !value.empty()) {
      options.set_value(option, value);
    } else if (kind == option_kind::flag) {
      options.set_value(option, false);
    }
  }

  return options;
}
