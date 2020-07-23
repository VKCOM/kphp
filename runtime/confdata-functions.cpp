#include "runtime/confdata-functions.h"

#include "runtime/confdata-global-manager.h"
#include "runtime/string_functions.h"

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

private:
  ConfdataLocalManager() :
    global_manager_{ConfdataGlobalManager::get()} {};

  ConfdataGlobalManager &global_manager_;
  const ConfdataSample *acquired_sample_{nullptr};
};

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

var f$confdata_get_value(const string &key) noexcept {
  if (!ConfdataLocalManager::get().is_initialized()) {
    php_warning("Confdata is not initialized");
    return {};
  }
  if (key.size() > std::numeric_limits<int16_t>::max()) {
    php_warning("Too long key");
    return {};
  }

  ConfdataKeyMaker confdata_key_maker{key.c_str(), static_cast<int16_t>(key.size())};
  const auto &confdata_storage = ConfdataLocalManager::get().get_confdata_storage();
  auto it = confdata_storage.find(confdata_key_maker.get_first_key());
  if (it == confdata_storage.end()) {
    return {};
  }
  // если ключ не имеет '.'
  if (confdata_key_maker.get_first_key_type() == ConfdataFirstKeyType::simple_key) {
    return it->second;
  }
  // тут обязательно должен быть массив, мы так загружали
  php_assert(it->second.is_array());
  return it->second.as_array().get_var(confdata_key_maker.get_second_key());
}

array<var> f$confdata_get_values_by_wildcard(string wildcard) noexcept {
  if (!ConfdataLocalManager::get().is_initialized()) {
    php_warning("Confdata is not initialized");
    return {};
  }
  if (wildcard.size() > std::numeric_limits<int16_t>::max()) {
    php_warning("Too long wildcard");
    return {};
  }
  if (auto star = static_cast<const char *>(memchr(wildcard.c_str(), '*', wildcard.size()))) {
    const auto star_pos = static_cast<string::size_type>(star - wildcard.c_str());
    if (star_pos + 1 != wildcard.size()) {
      php_warning("Wildcard star must be at the end");
      return {};
    }
    wildcard = string{wildcard.c_str(), star_pos};
  }

  // запрещяем получать всю конфдату
  if (wildcard.empty()) {
    php_warning("Empty wildcard is not supported");
    return {};
  }

  const auto &confdata_storage = ConfdataLocalManager::get().get_confdata_storage();
  ConfdataKeyMaker key_maker{wildcard.c_str(), static_cast<int16_t>(wildcard.size())};
  // wildcard имеет вид '\w+\..*' или '\w+\.\w+\..*'
  if (key_maker.get_first_key_type() != ConfdataFirstKeyType::simple_key) {
    // соотвественно первый ключ это или '\w+\.' или '\w+\.\w+\.'
    auto it = confdata_storage.find(key_maker.get_first_key());
    if (it == confdata_storage.end()) {
      return {};
    }

    // тут обязательно должен быть массив, мы так загружали
    php_assert(it->second.is_array());
    const auto &second_key_array = it->second.as_array();

    // если второй ключ это пустая строка, т.е. весь префикс это первый ключ ('\w+\.' или '\w+\.\w+\.')
    if (key_maker.get_second_key().is_string() && key_maker.get_second_key().as_string().empty()) {
      return second_key_array;
    }

    // если второй ключ не пустая строка, значит нам нужно соотвествующее подможество
    array<var> result;
    const auto second_key_prefix = key_maker.get_second_key().to_string();
    for (const auto &second_key_it : second_key_array) {
      const string key_str = second_key_it.get_key().to_string();
      if (key_str.starts_with(second_key_prefix)) {
        result.set_value(f$substr(key_str, second_key_prefix.size()).val(), second_key_it.get_value());
      }
    }
    return result;
  }

  // wildcard имеет вид '\w+'
  array<var> result;
  auto it = confdata_storage.lower_bound(wildcard);
  while (it != confdata_storage.end() && it->first.starts_with(wildcard)) {
    switch (key_maker.update(it->first.c_str(), static_cast<int16_t>(it->first.size()))) {
      case ConfdataFirstKeyType::simple_key:
        result.set_value(f$substr(it->first, wildcard.size()).val(), it->second);
        break;
      case ConfdataFirstKeyType::one_dot_wildcard: {
        const auto section_suffix = f$substr(it->first, wildcard.size()).val();
        php_assert(it->second.is_array());
        // тут обязательно должен быть массив, мы так загружали
        const auto &second_key_array = it->second.as_array();
        const auto inserting_size = second_key_array.size() + result.size();
        result.reserve(inserting_size.int_size, inserting_size.string_size, inserting_size.is_vector);
        for (const auto &section_it : it->second) {
          result.set_value(string{section_suffix}.append(section_it.get_key()), section_it.get_value());
        }
      }
      case ConfdataFirstKeyType::two_dots_wildcard:
        // тут мы ничего не делаем, потому что это является подмножеством элементов ConfdataFirstKeyType::one_dot_wildcard
        break;
    }
    ++it;
  }

  return result;
}
