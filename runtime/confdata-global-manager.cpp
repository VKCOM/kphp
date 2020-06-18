#include "runtime/confdata-global-manager.h"

#include "runtime/php_assert.h"

namespace {

void recursively_destroy_confdata_element(var &element) noexcept {
  if (element.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return;
  }
  if (element.is_array()) {
    auto &arr = element.as_array();
    auto it = arr.begin_no_mutate();
    auto last = arr.end_no_mutate();
    for (; it != last; ++it) {
      if (it.is_string_key() &&
          !it.get_string_key().is_reference_counter(ExtraRefCnt::for_global_const)) {
        it.get_string_key().force_destroy(ExtraRefCnt::for_confdata);
      }
      recursively_destroy_confdata_element(it.get_value());
    }
  } else if (!element.is_string()) {
    return;
  }
  element.force_destroy(ExtraRefCnt::for_confdata);
}

} // namespace

void ConfdataSample::init(memory_resource::unsynchronized_pool_resource &resource) noexcept {
  php_assert(!resource_);
  php_assert(!confdata_storage_);
  resource_ = &resource;
  auto *mem = resource_->allocate(sizeof(*confdata_storage_));
  php_assert(mem);
  confdata_storage_ = new(mem) confdata_sample_storage{confdata_sample_storage::allocator_type{*resource_}};
}

void ConfdataSample::reset(confdata_sample_storage &&new_confdata) noexcept {
  clear();
  *confdata_storage_ = std::move(new_confdata);
}

void ConfdataSample::clear() noexcept {
  php_assert(confdata_storage_);
  confdata_storage_->clear();

  if (garbage_) {
    garbage_->remove_if([](ConfdataGarbageNode &node) {
      if (node.destroy_way == ConfdataGarbageDestroyWay::shallow_first) {
        node.value.force_destroy(ExtraRefCnt::for_confdata);
        return true;
      }
      return false;
    });

    while (!garbage_->empty()) {
      recursively_destroy_confdata_element(garbage_->front().value);
      garbage_->pop_front();
    }
    delete garbage_;
    garbage_ = nullptr;
  }
}

void ConfdataSample::destroy() noexcept {
  php_assert(!resource_ == !confdata_storage_);
  if (resource_) {
    clear();
    confdata_storage_->~map();
    resource_->deallocate(confdata_storage_, sizeof(*confdata_storage_));

    confdata_storage_ = nullptr;
    resource_ = nullptr;
  }
}

void ConfdataSample::save_garbage(std::forward_list<ConfdataGarbageNode> &&garbage) noexcept {
  php_assert(!garbage_);
  php_assert(confdata_storage_);
  if (!garbage.empty()) {
    garbage_ = new std::forward_list<ConfdataGarbageNode>{std::move(garbage)};
  }
}

ConfdataGlobalManager &ConfdataGlobalManager::get() noexcept {
  static ConfdataGlobalManager manager;
  return manager;
}

void ConfdataGlobalManager::init(size_t confdata_memory_limit) noexcept {
  void *confdata_memory = mmap(nullptr, confdata_memory_limit,
                               PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  php_assert(confdata_memory);
  resource_.init(confdata_memory, confdata_memory_limit);
  confdata_samples_.init(resource_);
}

ConfdataGlobalManager::~ConfdataGlobalManager() noexcept {
  if (confdata_samples_.is_initial_process() && is_initialized()) {
    confdata_samples_.destroy();
    munmap(resource_.memory_begin(), resource_.get_memory_stats().memory_limit);
    resource_.init(nullptr, 0);
  }
}

ConfdataKeyMaker::ConfdataKeyMaker(const char *key, int16_t key_len) noexcept {
  update(key, key_len);
}

FirstKeyDots ConfdataKeyMaker::update(const char *key, int16_t key_len) noexcept {
  php_assert(key_len >= 0);
  raw_key_ = key;
  raw_key_len_ = key_len;
  first_key_ = string{};
  second_key_ = var{};

  const char *dot_one_end = std::find(key, key + key_len, '.');
  if (dot_one_end++ == key + key_len) {
    first_key_dots_ = FirstKeyDots::zero;
    first_key_ = string::make_const_string_on_memory(key, key_len, first_key_buffer_.data(), first_key_buffer_.size());
    return first_key_dots_;
  }
  const char *dot_two_end = std::find(dot_one_end, key + key_len, '.');
  const char *first_key_end = dot_one_end;
  if (dot_two_end++ == key + key_len) {
    first_key_dots_ = FirstKeyDots::one;
  } else {
    first_key_dots_ = FirstKeyDots::two;
    first_key_end = dot_two_end;
  }
  const auto first_key_len = static_cast<string::size_type>(first_key_end - key);
  first_key_ = string::make_const_string_on_memory(key, first_key_len, first_key_buffer_.data(), first_key_buffer_.size());
  key = first_key_end;
  key_len = static_cast<int16_t>(key_len - first_key_len);

  int key_as_int = 0;
  if (key_len && php_try_to_int(key, key_len, &key_as_int)) {
    second_key_ = key_as_int;
  } else {
    second_key_ = string::make_const_string_on_memory(key, static_cast<string::size_type>(key_len),
                                                      second_key_buffer_.data(), second_key_buffer_.size());
  }
  return first_key_dots_;
}

void ConfdataKeyMaker::forcibly_change_first_key_dots_from_two_to_one() noexcept {
  php_assert(first_key_dots_ == FirstKeyDots::two);
  const char *first_key_end = std::find(raw_key_, raw_key_ + raw_key_len_, '.') + 1;
  php_assert(first_key_end != raw_key_ + raw_key_len_ + 1);
  const auto first_key_len = static_cast<string::size_type>(first_key_end - raw_key_);
  const auto second_key_len = static_cast<string::size_type>(raw_key_len_ - first_key_len);
  first_key_ = string::make_const_string_on_memory(raw_key_, first_key_len, first_key_buffer_.data(), first_key_buffer_.size());
  second_key_ = string::make_const_string_on_memory(first_key_end, second_key_len, second_key_buffer_.data(), second_key_buffer_.size());
  first_key_dots_ = FirstKeyDots::one;
}
