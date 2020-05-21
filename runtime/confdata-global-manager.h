#pragma once
#include <forward_list>

#include "common/mixin/not_copyable.h"

#include "runtime/kphp_core.h"
#include "runtime/inter-process-resource.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

using confdata_sample_storage = memory_resource::stl::map<string, var, memory_resource::unsynchronized_pool_resource, stl_string_less>;

enum class ConfdataGarbageDestroyWay {
  shallow_first,
  deep_last
};

struct ConfdataGarbageNode {
  var value;
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

  void init(dl::size_type confdata_memory_limit) noexcept;

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

  ~ConfdataGlobalManager() noexcept;

private:
  ConfdataGlobalManager() = default;
  memory_resource::unsynchronized_pool_resource resource_;
  InterProcessResourceManager<ConfdataSample, 30> confdata_samples_;
};

enum class FirstKeyDots {
  zero,
  one,
  two,
};

class ConfdataKeyMaker {
public:
  ConfdataKeyMaker() = default;
  ConfdataKeyMaker(const char *key, int16_t key_len) noexcept;

  FirstKeyDots update(const char *key, int16_t key_len) noexcept;

  void forcibly_change_first_key_dots_from_two_to_one() noexcept;

  FirstKeyDots get_first_key_dots() const noexcept { return first_key_dots_; }

  const string &get_first_key() const noexcept { return first_key_; }
  const var &get_second_key() const noexcept { return second_key_; }

  string make_first_key_copy() const noexcept { return first_key_.copy_and_make_not_shared(); }
  var make_second_key_copy() const noexcept { return second_key_.is_string() ? var{second_key_.as_string().copy_and_make_not_shared()} : second_key_; }

private:
  const char *raw_key_{nullptr};
  short raw_key_len_{0};

  FirstKeyDots first_key_dots_{FirstKeyDots::zero};
  string first_key_;
  var second_key_;

  std::array<char, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> first_key_buffer_;
  std::array<char, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> second_key_buffer_;
};
