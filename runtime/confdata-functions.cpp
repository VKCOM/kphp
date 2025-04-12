// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/confdata-functions.h"

#include "common/algorithms/contains.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime/confdata-global-manager.h"

namespace {

class ConfdataLocalManager : vk::not_copyable {
public:
  static ConfdataLocalManager &get() {
    static ConfdataLocalManager confdata_local_manager;
    return confdata_local_manager;
  }

  void acquire_sample() noexcept {
    php_assert(!acquired_sample_);
    acquired_sample_ = global_manager_.acquire_current_sample();
  }

  void release_sample() noexcept {
    php_assert(acquired_sample_);
    global_manager_.release_sample(acquired_sample_);
    acquired_sample_ = nullptr;
  }

  const confdata_sample_storage &get_confdata_storage() const noexcept {
    php_assert(acquired_sample_);
    return acquired_sample_->get_confdata();
  }

  bool is_initialized() const noexcept {
    return global_manager_.is_initialized();
  }

  const ConfdataPredefinedWildcards &get_predefined_wildcards() const noexcept {
    return global_manager_.get_predefined_wildcards();
  }

  const ConfdataKeyBlacklist &get_key_blacklist() const noexcept {
    return global_manager_.get_key_blacklist();
  }

private:
  ConfdataLocalManager() :
    global_manager_{ConfdataGlobalManager::get()} {};

  ConfdataGlobalManager &global_manager_;
  const ConfdataSample *acquired_sample_{nullptr};
};

bool verify_confdata_key_param(const string &param, const char *real_name) noexcept {
  if (unlikely(!ConfdataLocalManager::get().is_initialized())) {
    php_warning("Confdata is not initialized");
    return false;
  }
  if (unlikely(param.size() > std::numeric_limits<int16_t>::max())) {
    php_warning("Too long %s", real_name);
    return false;
  }
  if (unlikely(param.empty())) {
    php_warning("Empty %s is not supported", real_name);
    return false;
  }
  return true;
}

} // namespace

void init_confdata_functions_lib() {
  if (ConfdataLocalManager::get().is_initialized()) {
    ConfdataLocalManager::get().acquire_sample();
  }
}

void free_confdata_functions_lib() {
  if (ConfdataLocalManager::get().is_initialized()) {
    ConfdataLocalManager::get().release_sample();
  }
}

bool f$is_confdata_loaded() noexcept {
  return ConfdataLocalManager::get().is_initialized();
}

mixed f$confdata_get_value(const string &key) noexcept {
  if (unlikely(!verify_confdata_key_param(key, "key"))) {
    return {};
  }

  const auto &local_manager = ConfdataLocalManager::get();
  ConfdataKeyMaker key_maker;
  key_maker.update(key.c_str(), static_cast<int16_t>(key.size()), local_manager.get_predefined_wildcards());
  const auto &confdata_storage = local_manager.get_confdata_storage();
  auto it = confdata_storage.find(key_maker.get_first_key());
  if (it != confdata_storage.end()) {
    // if key doesn't contain prefixes
    if (key_maker.get_first_key_type() == ConfdataFirstKeyType::simple_key) {
      return it->second;
    }
    // it must be an array (we loaded it this way)
    php_assert(it->second.is_array());
    if (const auto *value = it->second.as_array().find_value(key_maker.get_second_key())) {
      return *value;
    }
  }

  if (unlikely(local_manager.get_key_blacklist().is_blacklisted(vk::string_view{key.c_str(), key.size()}))) {
    php_warning("Trying to get blacklisted key '%s'", key.c_str());
  }
  return {};
}

array<mixed> f$confdata_get_values_by_any_wildcard(const string &wildcard) noexcept {
  if (unlikely(!verify_confdata_key_param(wildcard, "wildcard"))) {
    return {};
  }

  const auto &local_manager = ConfdataLocalManager::get();
  const auto &predefined_wildcards = local_manager.get_predefined_wildcards();
  ConfdataKeyMaker key_maker;
  const auto &confdata_storage = local_manager.get_confdata_storage();
  // wildcard has a form of '\w+\..*' or '\w+\.\w+\..*' and contains a predefined prefix
  if (key_maker.update(wildcard.c_str(), static_cast<int16_t>(wildcard.size()), predefined_wildcards) != ConfdataFirstKeyType::simple_key) {
    // the first key is '\w+\.' or '\w+\.\w+\.'
    auto it = confdata_storage.find(key_maker.get_first_key());
    if (it == confdata_storage.end()) {
      return {};
    }

    // it must be an array (we loaded it this way)
    php_assert(it->second.is_array());
    const auto &second_key_array = it->second.as_array();

    // if the second key is an empty string; i.e. the first key is an entire prefix ('\w+\.' or '\w+\.\w+\.' or predefined)
    if (key_maker.get_second_key().is_string() && key_maker.get_second_key().as_string().empty()) {
      return second_key_array;
    }

    // if the second key is not an empty string, then we need a prefix matching subset
    array<mixed> result;
    const auto second_key_prefix = key_maker.get_second_key().to_string();
    for (const auto &second_key_it : second_key_array) {
      const string key_str = second_key_it.get_key().to_string();
      if (key_str.starts_with(second_key_prefix)) {
        result.set_value(f$substr(key_str, second_key_prefix.size()).val(), second_key_it.get_value());
      }
    }
    return result;
  }

  // wildcard has a form of '\w+' and does not contain a predefined prefix
  array<mixed> result;
  auto merge_into_result = [&result, &wildcard](confdata_sample_storage::const_iterator iter) {
    const auto section_suffix = f$substr(iter->first, wildcard.size()).val();
    php_assert(iter->second.is_array());
    // it must be an array (we loaded it this way)
    const auto &second_key_array = iter->second.as_array();
    const auto inserting_size = second_key_array.size() + result.size();
    result.reserve(inserting_size.size, inserting_size.is_vector);
    for (const auto &section_it : iter->second) {
      result.set_value(string{section_suffix}.append(section_it.get_key()), section_it.get_value());
    }
  };
  auto it = confdata_storage.lower_bound(wildcard);
  while (it != confdata_storage.end() && it->first.starts_with(wildcard)) {
    const vk::string_view section_wildcard{it->first.c_str(), it->first.size()};
    switch (predefined_wildcards.detect_first_key_type(section_wildcard)) {
      case ConfdataFirstKeyType::simple_key:
        result.set_value(f$substr(it->first, wildcard.size()).val(), it->second);
        break;
      case ConfdataFirstKeyType::predefined_wildcard:
        // not a subset of any other prefixes
        if (!vk::contains(section_wildcard, ".") &&
            predefined_wildcards.is_most_common_predefined_wildcard(section_wildcard)) {
          merge_into_result(it);
        }
        break;
      case ConfdataFirstKeyType::one_dot_wildcard:
        // not a subset of any other predefined prefixes
        if (!predefined_wildcards.has_wildcard_for_key(section_wildcard)) {
          merge_into_result(it);
        }
        break;
      case ConfdataFirstKeyType::two_dots_wildcard:
        // a subset of ConfdataFirstKeyType::one_dot_wildcard
        break;
    }
    ++it;
  }

  return result;
}

array<mixed> f$confdata_get_values_by_predefined_wildcard(const string &wildcard) noexcept {
  if (unlikely(!verify_confdata_key_param(wildcard, "wildcard"))) {
    return {};
  }

  const auto &local_manager = ConfdataLocalManager::get();
  const auto &confdata_storage = local_manager.get_confdata_storage();
  const vk::string_view wildcard_view{wildcard.c_str(), wildcard.size()};
  if (local_manager.get_predefined_wildcards().detect_first_key_type(wildcard_view) == ConfdataFirstKeyType::simple_key) {
    php_warning("Trying to get elements by non predefined wildcard '%s'", wildcard.c_str());
    return {};
  }

  const auto elements_it = confdata_storage.find(wildcard);
  if (elements_it != confdata_storage.end()) {
    php_assert(elements_it->second.is_array());
    return elements_it->second.as_array();
  }
  return {};
}
