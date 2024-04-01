// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/confdata-global-manager.h"

#include "common/wrappers/memory-utils.h"
#include "runtime/php_assert.h"

namespace {

void recursively_destroy_confdata_element(mixed &element) noexcept {
  if (element.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return;
  }
  if (element.is_array()) {
    auto &arr = element.as_array();
    auto it = arr.begin_no_mutate();
    auto last = arr.end_no_mutate();
    for (; it != last; ++it) {
      if (it.is_string_key() && !it.get_string_key().is_reference_counter(ExtraRefCnt::for_global_const)) {
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
  confdata_storage_ = new (mem) confdata_sample_storage{confdata_sample_storage::allocator_type{*resource_}};
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

void ConfdataGlobalManager::init(size_t confdata_memory_limit, std::unordered_set<vk::string_view> &&predefined_wilrdcards,
                                 std::unique_ptr<re2::RE2> &&blacklist_pattern, std::forward_list<vk::string_view> &&force_ignore_prefixes) noexcept {
  resource_.init(mmap_shared(confdata_memory_limit), confdata_memory_limit);
  confdata_samples_.init(resource_);
  predefined_wildcards_.set_wildcards(std::move(predefined_wilrdcards));
  key_blacklist_.set_blacklist(std::move(blacklist_pattern), std::move(force_ignore_prefixes));
}

ConfdataGlobalManager::~ConfdataGlobalManager() noexcept {
  if (confdata_samples_.is_initial_process() && is_initialized()) {
    confdata_samples_.destroy();
    munmap(resource_.memory_begin(), resource_.get_memory_stats().memory_limit);
    resource_.init(nullptr, 0);
  }
}
