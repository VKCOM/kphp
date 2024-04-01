// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <forward_list>
#include <unordered_set>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

#include "runtime/confdata-keys.h"
#include "runtime/inter-process-resource.h"
#include "runtime/kphp_core.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

using confdata_sample_storage = memory_resource::stl::map<string, mixed, memory_resource::unsynchronized_pool_resource, stl_string_less>;

enum class ConfdataGarbageDestroyWay { shallow_first, deep_last };

struct ConfdataGarbageNode {
  mixed value;
  ConfdataGarbageDestroyWay destroy_way;
};

class ConfdataSample : vk::not_copyable {
public:
  void init(memory_resource::unsynchronized_pool_resource &resource) noexcept;
  void reset(confdata_sample_storage &&new_confdata) noexcept;
  void clear() noexcept;
  void destroy() noexcept;

  void save_garbage(std::forward_list<ConfdataGarbageNode> &&garbage) noexcept;

  const auto &get_confdata() const noexcept {
    return *confdata_storage_;
  }

private:
  memory_resource::unsynchronized_pool_resource *resource_{nullptr};
  confdata_sample_storage *confdata_storage_{nullptr};
  std::forward_list<ConfdataGarbageNode> *garbage_{nullptr};
};

class ConfdataGlobalManager : vk::not_copyable {
public:
  static ConfdataGlobalManager &get() noexcept;

  void init(size_t confdata_memory_limit, std::unordered_set<vk::string_view> &&predefined_wilrdcards, std::unique_ptr<re2::RE2> &&blacklist_pattern,
            std::forward_list<vk::string_view> &&force_ignore_prefixes) noexcept;

  void force_release_all_resources_acquired_by_this_proc_if_init() noexcept {
    if (is_initialized()) {
      confdata_samples_.force_release_all_resources();
    }
  }

  ConfdataSample &get_current() noexcept {
    return confdata_samples_.get_current_resource();
  }

  const ConfdataSample *acquire_current_sample() noexcept {
    return confdata_samples_.acquire_current_resource();
  }

  void release_sample(const ConfdataSample *sample) noexcept {
    return confdata_samples_.release_resource(sample);
  }

  bool can_next_be_updated() noexcept {
    return confdata_samples_.is_next_resource_unused();
  }

  bool try_switch_to_next_sample(confdata_sample_storage &&confdata_storage) noexcept {
    return confdata_samples_.try_switch_to_next_unused_resource(std::move(confdata_storage));
  }

  void clear_unused_samples() noexcept {
    return confdata_samples_.clear_dirty_unused_resources_in_sequence();
  }

  memory_resource::unsynchronized_pool_resource &get_resource() noexcept {
    return resource_;
  }

  bool is_initialized() const noexcept {
    return resource_.memory_begin();
  }

  const ConfdataPredefinedWildcards &get_predefined_wildcards() const noexcept {
    return predefined_wildcards_;
  }

  const ConfdataKeyBlacklist &get_key_blacklist() const noexcept {
    return key_blacklist_;
  }

  ~ConfdataGlobalManager() noexcept;

private:
  ConfdataGlobalManager() = default;
  memory_resource::unsynchronized_pool_resource resource_;
  InterProcessResourceManager<ConfdataSample, 30> confdata_samples_;

  ConfdataPredefinedWildcards predefined_wildcards_;
  ConfdataKeyBlacklist key_blacklist_;
};
