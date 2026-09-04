// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-functions.h"

#include <cstddef>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/confdata/confdata-keys.h"
#include "runtime-light/stdlib/confdata/confdata-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {

auto verify_confdata_parameter(std::string_view parameter) noexcept -> bool {
  if (!ConfdataInstanceState::get().is_initialized()) [[unlikely]] {
    kphp::log::warning("confdata is not initialized");
    return false;
  }
  if (parameter.size() > kphp::confdata::MAX_KEY_LENGTH) [[unlikely]] {
    kphp::log::warning("confdata key is too long {}", parameter);
    return false;
  }
  if (parameter.empty()) [[unlikely]] {
    kphp::log::warning("confdata does not support empty keys");
    return false;
  }
  return true;
}

} // namespace

auto f$confdata_get_value(const string& key) noexcept -> mixed {
  const std::string_view key_view{key.c_str(), key.size()};
  if (!verify_confdata_parameter(key_view)) [[unlikely]] {
    return {};
  }

  const auto& confdata_st{ConfdataInstanceState::get()};
  const auto views{kphp::confdata::split_key(key_view, confdata_st.wildcards())};
  kphp::log::assertion(views.has_value());
  const kphp::confdata::key_handles handles{*views};

  const auto& values{confdata_st.values()};
  const auto section_it{values.find(handles.section())};
  if (section_it == values.end()) {
    return {};
  }
  if (views->kind() == kphp::confdata::section_kind::simple_key) {
    return section_it->second;
  }

  kphp::log::assertion(section_it->second.is_array());
  if (const auto* value{section_it->second.as_array().find_value(handles.remainder())}; value != nullptr) {
    return *value;
  }
  return {};
}

auto f$confdata_get_values_by_any_wildcard(const string& wildcard) noexcept -> array<mixed> {
  const std::string_view wildcard_view{wildcard.c_str(), wildcard.size()};
  if (!verify_confdata_parameter(wildcard_view)) [[unlikely]] {
    return {};
  }

  const auto& confdata_st{ConfdataInstanceState::get()};
  const auto& predefined_wildcards{confdata_st.wildcards()};
  const auto views{kphp::confdata::split_key(wildcard_view, predefined_wildcards)};
  kphp::log::assertion(views.has_value());
  const kphp::confdata::key_handles handles{*views};
  const auto& values{confdata_st.values()};

  if (views->kind() != kphp::confdata::section_kind::simple_key) {
    const auto section_it{values.find(handles.section())};
    if (section_it == values.end()) {
      return {};
    }

    kphp::log::assertion(section_it->second.is_array());
    const auto& entries{section_it->second.as_array()};
    if (handles.remainder().is_string() && handles.remainder().as_string().empty()) {
      return entries;
    }

    array<mixed> result{};
    const string remainder_prefix{handles.remainder().to_string()};
    const std::string_view remainder_prefix_view{remainder_prefix.c_str(), remainder_prefix.size()};
    for (const auto& entry : entries) {
      const string entry_key{entry.get_key().to_string()};
      const std::string_view entry_key_view{entry_key.c_str(), entry_key.size()};
      if (entry_key_view.starts_with(remainder_prefix_view)) {
        const auto suffix{entry_key_view.substr(remainder_prefix_view.size())};
        result.set_value(string{suffix.data(), static_cast<string::size_type>(suffix.size())}, entry.get_value());
      }
    }
    return result;
  }

  array<mixed> result{};
  const auto merge_entries{[&result, wildcard_view](kphp::confdata::storage::map_type::const_iterator section_it) noexcept {
    const std::string_view section_view{section_it->first.c_str(), section_it->first.size()};
    const auto suffix_view{section_view.substr(wildcard_view.size())};
    const string section_suffix{suffix_view.data(), static_cast<string::size_type>(suffix_view.size())};
    kphp::log::assertion(section_it->second.is_array());
    const auto& entries{section_it->second.as_array()};
    const auto inserting_size{entries.size() + result.size()};
    result.reserve(inserting_size.size, inserting_size.is_vector);
    for (const auto& entry : entries) {
      result.set_value(string{section_suffix}.append(entry.get_key()), entry.get_value());
    }
  }};

  auto section_it{values.lower_bound(handles.section())};
  for (std::string_view section_view{section_it->first.c_str(), section_it->first.size()};
       section_it != values.end() && section_view.starts_with(wildcard_view);) {
    section_view = {section_it->first.c_str(), section_it->first.size()};
    switch (kphp::confdata::classify_section(section_view, predefined_wildcards)) {
    case kphp::confdata::section_kind::simple_key: {
      const auto suffix{section_view.substr(wildcard_view.size())};
      result.set_value(string{suffix.data(), static_cast<string::size_type>(suffix.size())}, section_it->second);
      break;
    }
    case kphp::confdata::section_kind::predefined_wildcard:
      if (!section_view.contains('.') && predefined_wildcards.is_top_level_wildcard(section_view)) {
        merge_entries(section_it);
      }
      break;
    case kphp::confdata::section_kind::one_dot_wildcard:
      if (!predefined_wildcards.has_matching_wildcard(section_view)) {
        merge_entries(section_it);
      }
      break;
    case kphp::confdata::section_kind::two_dots_wildcard:
      break;
    }
    ++section_it;
  }
  return result;
}

auto f$confdata_get_values_by_predefined_wildcard(const string& wildcard) noexcept -> array<mixed> {
  const std::string_view wildcard_view{wildcard.c_str(), wildcard.size()};
  if (!verify_confdata_parameter(wildcard_view)) [[unlikely]] {
    return {};
  }

  const auto& confdata_st{ConfdataInstanceState::get()};
  if (kphp::confdata::classify_section(wildcard_view, confdata_st.wildcards()) == kphp::confdata::section_kind::simple_key) [[unlikely]] {
    kphp::log::warning("trying to get elements by non-predefined wildcard '{}'", wildcard_view);
    return {};
  }

  const auto views{kphp::confdata::split_key_with_predefined_wildcard(wildcard_view, wildcard_view.size())};
  kphp::log::assertion(views.has_value());
  const kphp::confdata::key_handles handles{*views};
  const auto& values{confdata_st.values()};
  const auto elements_it{values.find(handles.section())};
  if (elements_it == values.end()) {
    return {};
  }

  kphp::log::assertion(elements_it->second.is_array());
  return elements_it->second.as_array();
}
