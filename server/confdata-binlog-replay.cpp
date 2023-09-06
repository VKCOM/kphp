// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/confdata-binlog-replay.h"

#include <bitset>
#include <chrono>
#include <cinttypes>
#include <forward_list>
#include <map>

#include "common/binlog/binlog-replayer.h"
#include "common/dl-utils-lite.h"
#include "common/precise-time.h"
#include "common/server/engine-settings.h"
#include "common/server/init-binlog.h"
#include "common/server/init-snapshot.h"
#include "common/wrappers/string_view.h"
#include "common/kfs/kfs.h"

#include "runtime/allocator.h"
#include "runtime/confdata-global-manager.h"
#include "runtime/kphp_core.h"
#include "server/confdata-binlog-events.h"
#include "server/confdata-stats.h"
#include "server/server-log.h"

namespace {

class ConfdataBinlogReplayer : vk::binlog::replayer {
public:
  enum class OperationStatus {
    no_update,
    blacklisted,
    ttl_update_only,
    full_update
  };

  static ConfdataBinlogReplayer &get() noexcept {
    static ConfdataBinlogReplayer loader;
    return loader;
  }

  using vk::binlog::replayer::replay;

  size_t try_reserve_for_snapshot(vk::string_view key, size_t search_from,
                                  vk::string_view &prev_key, array_size &counter) noexcept {
    auto dot_pos = key.find('.', search_from);
    if (dot_pos != std::string::npos) {
      const auto dot_key = key.substr(0, dot_pos + 1);
      if (prev_key != dot_key) {
        if (counter.size > 1) {
          size_hints_[prev_key] = counter;
        }
        prev_key = dot_key;
        counter = array_size{};
      }
      ++counter.size;

    }
    return dot_pos;
  }

  int load_index() noexcept {
    if (!Snapshot) {
      jump_log_ts = 0;
      jump_log_pos = 0;
      jump_log_crc32 = 0;
      return 0;
    }
    index_header header;
    kfs_read_file_assert (Snapshot, &header, sizeof(index_header));
    if (header.magic != PMEMCACHED_INDEX_MAGIC) {
      fprintf(stderr, "index file is not for confdata\n");
      return -1;
    }
    jump_log_ts = header.log_timestamp;
    jump_log_pos = header.log_pos1;
    jump_log_crc32 = header.log_pos1_crc32;

    const int nrecords = header.nrecords;
    vkprintf(2, "%d records readed\n", nrecords);
    auto index_offset = std::make_unique<int64_t[]>(nrecords + 1);
    assert (index_offset);

    kfs_read_file_assert (Snapshot, index_offset.get(), sizeof(index_offset[0]) * (nrecords + 1));
    vkprintf(1, "index_offset[%d]=%" PRId64 "\n", nrecords, index_offset[nrecords]);

    auto index_binary_data = std::make_unique<char[]>(index_offset[nrecords]);
    assert (index_binary_data);
    kfs_read_file_assert (Snapshot, index_binary_data.get(), index_offset[nrecords]);

    using entry_type = lev_confdata_store_wrapper<index_entry, pmct_set>;

    vk::string_view last_one_dot_key;
    vk::string_view last_two_dots_key;
    array_size one_dot_elements_counter;
    array_size two_dots_elements_counter;
    for (int i = 0; i < nrecords; i++) {
      const auto &element = reinterpret_cast<const entry_type &>(index_binary_data[index_offset[i]]);
      const vk::string_view key{element.data, static_cast<size_t>(std::max(element.key_len, short{0}))};
      if (key.empty() || key_blacklist_.is_blacklisted(key)) {
        index_offset[i] = -1;
        ++event_counters_.snapshot_entry.blacklisted;
      } else {
        const auto first_dot = try_reserve_for_snapshot(key, 0, last_one_dot_key, one_dot_elements_counter);
        if (first_dot != std::string::npos) {
          try_reserve_for_snapshot(key, first_dot + 1, last_two_dots_key, two_dots_elements_counter);
        }
      }
      ++event_counters_.snapshot_entry.total;
    }

    // disable the blacklist because we checked the keys during the previous step
    blacklist_enabled_ = false;
    for (int i = 0; i < nrecords; i++) {
      if (index_offset[i] >= 0) {
        store_element(reinterpret_cast<const entry_type &>(index_binary_data[index_offset[i]]));
      }
    }
    blacklist_enabled_ = true;
    size_hints_.clear();
    return 0;
  }

  OperationStatus delete_element(const char *key, short key_len) noexcept {
    return generic_operation(key, key_len, -1, [this] { return delete_processing_element(); });
  }

  OperationStatus touch_element(const lev_confdata_touch &E) noexcept {
    return generic_operation(E.key, static_cast<short>(E.key_len), E.delay, [this] { return touch_processing_element(); });
  }

  template<class BASE, int OPERATION>
  OperationStatus store_element(const lev_confdata_store_wrapper<BASE, OPERATION> &E) noexcept {
    // don't even try to capture E by value; it'll try to copy it and it would be very sad :(
    return generic_operation(E.data, E.key_len, E.get_delay(), [this, &E] { return store_processing_element(E); });
  }

  void unsupported_operation(const char *operation_name, const char *key, int key_len) noexcept {
    ++event_counters_.unsupported_total_events;
    log_server_warning("Confdata binlog reading error: got unsupported operation '%s' with key '%.*s'", operation_name, std::max(key_len, 0), key);
  }

  void init(memory_resource::unsynchronized_pool_resource &memory_pool) noexcept {
    assert(!updating_confdata_storage_);
    updating_confdata_storage_ = new(&confdata_mem_)confdata_sample_storage{confdata_sample_storage::allocator_type{memory_pool}};
  }

  struct ConfdataUpdateResult {
    confdata_sample_storage new_confdata;
    std::forward_list<ConfdataGarbageNode> previous_confdata_garbage;
    size_t previous_confdata_garbage_size;
  };

  ConfdataUpdateResult finish_confdata_update() noexcept {
    for (auto &confdata_section: *updating_confdata_storage_) {
      // save into the separate variable to avoid the const_cast
      string key = confdata_section.first;
      mark_string_as_confdata_const(key);
      if (confdata_section.second.is_array()) {
        mark_array_as_confdata_const(confdata_section.second.as_array());
      } else if (confdata_section.second.is_string()) {
        mark_string_as_confdata_const(confdata_section.second.as_string());
      }
    }

    ConfdataUpdateResult result{
      std::move(*updating_confdata_storage_),
      std::move(*garbage_from_previous_confdata_sample_),
      garbage_size_
    };
    // do an explicit clear() as a container is left in "a valid but unspecified state" after the move
    updating_confdata_storage_->clear();
    garbage_from_previous_confdata_sample_->clear();
    garbage_size_ = 0;
    confdata_has_any_updates_ = false;
    return result;
  }

  void try_use_previous_confdata_storage_as_init(const confdata_sample_storage &previous_confdata_storage) noexcept {
    if (!confdata_has_any_updates_) {
      assert(garbage_from_previous_confdata_sample_->empty());
      if (updating_confdata_storage_->empty()) {
        *updating_confdata_storage_ = previous_confdata_storage;
      } else {
        // strictly speaking, they should be identical, but it's too hard to verify
        dl_assert(updating_confdata_storage_->size() == previous_confdata_storage.size(),
                  dl_pstr("Can't update confdata from binlog: 'updating_confdata' and 'previous_confdata' must be identical, "
                          "but they have different sizes (%zu != %zu)\n", updating_confdata_storage_->size(), previous_confdata_storage.size()));
      }
    }
  }

  bool has_new_confdata() const noexcept {
    return confdata_has_any_updates_;
  }

  void delete_expired_elements() noexcept {
    assert(expiration_trace_.size() == element_delays_.size());

    size_t expired_elements = expiration_trace_.size();
    while (!expiration_trace_.empty()) {
      auto head = expiration_trace_.begin();
      if (head->first > get_now()) {
        return;
      }
      assert(head->second.size() <= std::numeric_limits<short>::max());
      delete_element(head->second.c_str(), static_cast<short>(head->second.size()));
      assert(expired_elements == expiration_trace_.size() + 1);
      expired_elements = expiration_trace_.size();
    }
  }

  size_t get_elements_with_delay_count() const noexcept {
    return element_delays_.size();
  }

  const ConfdataStats::EventCounters &get_event_counters() const noexcept {
    return event_counters_;
  }

private:
  ConfdataBinlogReplayer() noexcept:
    garbage_from_previous_confdata_sample_(new(&garbage_mem_) GarbageList{}),
    key_blacklist_(ConfdataGlobalManager::get().get_key_blacklist()),
    predefined_wildcards_(ConfdataGlobalManager::get().get_predefined_wildcards()) {
    add_handler([this](const lev_confdata_delete &E) {
      update_event_stat(this->delete_element(E.key, E.key_len), event_counters_.delete_events);
    });
    add_handler([this](const lev_confdata_touch &E) {
      update_event_stat(this->touch_element(E), event_counters_.touch_events);
    });

    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_add> &E) {
      update_event_stat(this->store_element(E), event_counters_.add_events);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_set> &E) {
      update_event_stat(this->store_element(E), event_counters_.set_events);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_replace> &E) {
      update_event_stat(this->store_element(E), event_counters_.replace_events);
    });

    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_add> &E) {
      update_event_stat(this->store_element(E), event_counters_.add_forever_events);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_set> &E) {
      update_event_stat(this->store_element(E), event_counters_.set_forever_events);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_replace> &E) {
      update_event_stat(this->store_element(E), event_counters_.replace_forever_events);
    });

    add_handler([this](const lev_confdata_get &E) {
      ++event_counters_.get_events;
      this->unsupported_operation("get", E.key, E.key_len);
    });
    add_handler([this](const lev_confdata_incr &E) {
      ++event_counters_.incr_events;
      this->unsupported_operation("incr", E.key, E.key_len);
    });
    add_handler_range([this](const lev_confdata_incr_tiny_range &E) {
      ++event_counters_.incr_tiny_events;
      this->unsupported_operation("incr tiny", E.key, E.key_len);
    });
    add_handler([this](const lev_confdata_append &E) {
      ++event_counters_.append_events;
      this->unsupported_operation("append", E.data, E.key_len);
    });
  }

  template<typename F>
  OperationStatus generic_operation(const char *key, short key_len, int delay, const F &operation) noexcept {
    // TODO assert?
    if (key_len < 0) {
      return OperationStatus::blacklisted;
    }
    const vk::string_view key_view{key, static_cast<size_t>(key_len)};
    if (is_key_blacklisted(key_view)) {
      return OperationStatus::blacklisted;
    }

    assert(last_element_in_garbage_.is_null());
    assert(processing_value_.is_null());
    static_assert(std::is_same<short, int16_t>{}, "short is expected to be int16_t");

    OperationStatus last_operation_status{OperationStatus::no_update};
    const auto predefined_wildcard_lengths = predefined_wildcards_.make_predefined_wildcard_len_range_by_key(key_view);
    for (size_t wildcard_len : predefined_wildcard_lengths) {
      assert(wildcard_len <= std::numeric_limits<int16_t>::max());
      processing_key_.update_with_predefined_wildcard(key, key_len, static_cast<int16_t>(wildcard_len));
      const auto operation_status = operation();
      assert(last_operation_status != OperationStatus::full_update ||
             operation_status == OperationStatus::full_update);
      last_operation_status = operation_status;
      if (operation_status != OperationStatus::full_update) {
        break;
      }
    }

    if (predefined_wildcard_lengths.empty() ||
        last_operation_status == OperationStatus::full_update) {
      const auto first_key_type = processing_key_.update(key, key_len);
      if (predefined_wildcard_lengths.empty() ||
          first_key_type != ConfdataFirstKeyType::simple_key) {
        const auto operation_status = operation();
        assert(last_operation_status != OperationStatus::full_update ||
               operation_status == OperationStatus::full_update);
        if (operation_status == OperationStatus::full_update &&
            first_key_type == ConfdataFirstKeyType::two_dots_wildcard) {
          processing_key_.forcibly_change_first_key_wildcard_dots_from_two_to_one();
          const auto should_be_full = operation();
          assert(should_be_full == OperationStatus::full_update);
        }
        last_operation_status = operation_status;
      }
    }
    if (last_operation_status == OperationStatus::no_update) {
      assert(last_element_in_garbage_.is_null());
      assert(processing_value_.is_null());
      return last_operation_status;
    }

    last_element_in_garbage_.clear();
    processing_value_.clear();
    if (last_operation_status == OperationStatus::full_update) {
      confdata_has_any_updates_ = true;
    }
    update_element_in_expiration_trace(key_view, delay);
    return last_operation_status;
  }

  static void update_event_stat(OperationStatus status, ConfdataStats::EventCounters::Event &event) noexcept {
    ++event.total;
    switch (status) {
      case OperationStatus::no_update:
        ++event.ignored;
        return;
      case OperationStatus::blacklisted:
        ++event.blacklisted;
        return;
      case OperationStatus::ttl_update_only:
        ++event.ttl_updated;
        return;
      case OperationStatus::full_update:
        return;
    }
  }

  OperationStatus delete_processing_element() noexcept {
    auto first_key_it = updating_confdata_storage_->find(processing_key_.get_first_key());
    if (first_key_it == updating_confdata_storage_->end()) {
      return OperationStatus::no_update;
    }

    // for keys without '.'
    if (processing_key_.get_first_key_type() == ConfdataFirstKeyType::simple_key) {
      // move deleted element key and value to garbage
      put_confdata_element_value_into_garbage(first_key_it->second);
      put_confdata_var_into_garbage(first_key_it->first, ConfdataGarbageDestroyWay::shallow_first);
      updating_confdata_storage_->erase(first_key_it);
      return OperationStatus::full_update;
    }

    assert(first_key_it->second.is_array());
    auto &array_for_second_key = first_key_it->second.as_array();
    if (!array_for_second_key.has_key(processing_key_.get_second_key())) {
      return OperationStatus::no_update;
    }

    // move deleted element data to garbage; it will be a copy with RC detachment
    put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);

    array_for_second_key.mutate_if_shared();
    auto second_key_it = array_for_second_key.find_no_mutate(processing_key_.get_second_key());
    assert(second_key_it != array_for_second_key.end());
    // array RC should be detached
    assert(array_for_second_key.get_reference_counter() == 1);
    // put key and value to garbage
    if (second_key_it.is_string_key() && second_key_it.get_string_key().get_reference_counter() != 1) {
      // key is removed with deep_last option, as it must be removed after array_for_second_key
      put_confdata_var_into_garbage(second_key_it.get_string_key(), ConfdataGarbageDestroyWay::deep_last);
    }
    put_confdata_element_value_into_garbage(second_key_it.get_value());
    array_for_second_key.unset(processing_key_.get_second_key());
    if (array_for_second_key.empty()) {
      // name is moved to garbage, the section itself will be removed due to RC detachment (see above)
      put_confdata_var_into_garbage(first_key_it->first, ConfdataGarbageDestroyWay::shallow_first);
      updating_confdata_storage_->erase(first_key_it);
    }
    return OperationStatus::full_update;
  }

  OperationStatus touch_processing_element() noexcept {
    auto first_key_it = updating_confdata_storage_->find(processing_key_.get_first_key());
    if (first_key_it == updating_confdata_storage_->end()) {
      return OperationStatus::no_update;
    }

    // for keys without '.'
    if (processing_key_.get_first_key_type() == ConfdataFirstKeyType::simple_key) {
      return OperationStatus::ttl_update_only;
    }

    assert(first_key_it->second.is_array());
    auto &array_for_second_key = first_key_it->second.as_array();
    return array_for_second_key.has_key(processing_key_.get_second_key())
           ? OperationStatus::ttl_update_only
           : OperationStatus::no_update;
  }

  template<class BASE, int OPERATION>
  OperationStatus store_processing_element(const lev_confdata_store_wrapper<BASE, OPERATION> &E) noexcept {
    auto first_key_it = updating_confdata_storage_->find(processing_key_.get_first_key());
    bool element_exists = true;
    if (first_key_it == updating_confdata_storage_->end()) {
      element_exists = false;
      // during the snapshot loading all keys are already sorted, so it makes sense to push it back
      first_key_it = updating_confdata_storage_->emplace_hint(first_key_it, processing_key_.make_first_key_copy(), mixed{});
    }

    // for keys without '.'
    if (processing_key_.get_first_key_type() == ConfdataFirstKeyType::simple_key) {
      if (!can_element_be_saved(E, element_exists)) {
        return OperationStatus::no_update;
      }

      if (is_new_value(E, first_key_it->second)) {
        // put the previous value to garbage and overwrite it with a new value
        put_confdata_element_value_into_garbage(first_key_it->second);
        first_key_it->second = get_processing_value(E);
        return OperationStatus::full_update;
      }
      return OperationStatus::ttl_update_only;
    }

    // null is inserted by the default
    if (first_key_it->second.is_null()) {
      first_key_it->second = prepare_array_for(vk::string_view{first_key_it->first.c_str(), first_key_it->first.size()});
    }
    assert(first_key_it->second.is_array());
    auto &array_for_second_key = first_key_it->second.as_array();
    const auto *prev_value = element_exists ? array_for_second_key.find_value(processing_key_.get_second_key()) : nullptr;
    if (!can_element_be_saved(E, prev_value != nullptr)) {
      return OperationStatus::no_update;
    }

    if (!prev_value) {
      // move old element data to garbage; it will be a copy with RC detachment
      put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);
      array_for_second_key.set_value(processing_key_.make_second_key_copy(), get_processing_value(E));
      // array insertion will detach the array RC
      assert(array_for_second_key.get_reference_counter() == 1);
      return OperationStatus::full_update;
    }
    if (is_new_value(E, *prev_value)) {
      // move old element data to garbage; it will be a copy with RC detachment
      put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);
      array_for_second_key.mutate_if_shared();
      auto second_key_it = array_for_second_key.find_no_mutate(processing_key_.get_second_key());
      assert(second_key_it != array_for_second_key.end());
      // array RC should be detached
      assert(array_for_second_key.get_reference_counter() == 1);
      // put the previous value to garbage and overwrite it with a new value
      put_confdata_element_value_into_garbage(second_key_it.get_value());
      second_key_it.get_value() = get_processing_value(E);
      return OperationStatus::full_update;
    }
    return OperationStatus::ttl_update_only;
  }

  void put_confdata_element_value_into_garbage(const mixed &element) noexcept {
    if (!last_element_in_garbage_.is_null()) {
      // if keys with 1 or 2 can refer the same element,
      // they should have identical internal pointers here as well
      assert(last_element_in_garbage_.get_type() == element.get_type());
      if (element.is_string()) {
        assert(element.as_string().c_str() == last_element_in_garbage_.as_string().c_str());
      } else if (element.is_array()) {
        assert(element.as_array().is_equal_inner_pointer(last_element_in_garbage_.as_array()));
      } else {
        assert(!"Unexpected type!");
      }
      return;
    }
    if ((element.is_string() || element.is_array()) &&
        (element.is_reference_counter(ExtraRefCnt::for_confdata) ||
         element.is_reference_counter(ExtraRefCnt::for_global_const))) {
      put_confdata_var_into_garbage(element, ConfdataGarbageDestroyWay::deep_last);
      last_element_in_garbage_ = element;
    }
  }

  template<typename T>
  void put_confdata_var_into_garbage(const T &v, ConfdataGarbageDestroyWay destroy_way) noexcept {
    if (v.is_reference_counter(ExtraRefCnt::for_confdata)) {
      garbage_from_previous_confdata_sample_->emplace_front(ConfdataGarbageNode{v, destroy_way});
      ++garbage_size_;
      return;
    }
    if (v.is_reference_counter(ExtraRefCnt::for_global_const) ||
        (destroy_way == ConfdataGarbageDestroyWay::shallow_first && v.get_reference_counter() == 1)) {
      return;
    }
    assert(!"Got unexpected reference counter");
  }

  template<class T>
  void assert_correct_ref_counter(const T &element) const noexcept {
    const auto refcnt = element.get_reference_counter();
    // 2 is the default wildcard max number + max number of the custom wildcards
    assert(refcnt > 0 && refcnt <= 2 + predefined_wildcards_.get_max_wildcards_for_element());
  }

  void mark_string_as_confdata_const(string &str) const noexcept {
    if (str.is_reference_counter(ExtraRefCnt::for_confdata) ||
        str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return;
    }
    assert_correct_ref_counter(str);
    str.set_reference_counter_to(ExtraRefCnt::for_confdata);
  }

  void mark_array_as_confdata_const(array<mixed> &arr) const noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_confdata) ||
        arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return;
    }
    assert_correct_ref_counter(arr);
    arr.set_reference_counter_to(ExtraRefCnt::for_confdata);
    const auto last = arr.end_no_mutate();
    for (auto it = arr.begin_no_mutate(); it != last; ++it) {
      if (it.is_string_key()) {
        mark_string_as_confdata_const(it.get_string_key());
      }
      auto &value = it.get_value();
      if (value.is_array()) {
        mark_array_as_confdata_const(value.as_array());
      } else if (value.is_string()) {
        mark_string_as_confdata_const(value.as_string());
      }
    }
  }

  template<class BASE, int OPERATION>
  bool can_element_be_saved(const lev_confdata_store_wrapper<BASE, OPERATION> &E, bool element_exists) const noexcept {
    static_assert(vk::any_of_equal(OPERATION, pmct_add, pmct_set, pmct_replace), "add or set or replace are expected");
    if (OPERATION == pmct_set) {
      return true;
    }
    if (!element_exists) {
      // if the element doesn't exist, we can do pmct_add, but we can't do pmct_replace
      return OPERATION == pmct_add;
    }
    // if the element exists, check whether it's not expired
    vk::string_view key{E.data, static_cast<size_t>(E.key_len)};
    auto it = element_delays_.find(key);
    // if the element is not inside element_delays_ then it's not expired yet
    const bool element_outdated = it != element_delays_.end() && it->second < get_now();
    return element_outdated ? OPERATION == pmct_add : OPERATION == pmct_replace;
  }

  void update_element_in_expiration_trace(vk::string_view key, int delay) noexcept {
    std::string removed_key = remove_element_from_expiration_trace(key);
    if (delay < 0) {
      return;
    }
    if (removed_key.empty()) {
      removed_key.assign(key.data(), key.size());
    } else {
      assert(vk::string_view{removed_key} == key);
    }
    auto it = expiration_trace_.emplace(delay, std::move(removed_key));
    const bool inserted = element_delays_.emplace(vk::string_view{it->second}, delay).second;
    assert(inserted);
  }

  std::string remove_element_from_expiration_trace(vk::string_view key) noexcept {
    std::string removed_key;
    auto expired_at_it = element_delays_.find(key);
    if (expired_at_it != element_delays_.end()) {
      auto range = expiration_trace_.equal_range(expired_at_it->second);
      auto it = std::find_if(range.first, range.second,
                             [&key](const std::pair<const int, std::string> &expired_key) {
                               return vk::string_view{expired_key.second} == key;
                             });
      assert(it != range.second);
      element_delays_.erase(expired_at_it);
      removed_key = std::move(it->second);
      expiration_trace_.erase(it);
    }
    return removed_key;
  }

  template<class BASE, int OPERATION>
  bool is_new_value(const lev_confdata_store_wrapper<BASE, OPERATION> &E, const mixed &prev_value) noexcept {
    if (E.get_flags()) {
      return !equals(get_processing_value(E), prev_value);
    }
    // (E.get_flags() == 0) -> new value is a string
    if (!prev_value.is_string()) {
      return true;
    }
    const auto &prev_str_value = prev_value.as_string();
    return vk::string_view{prev_str_value.c_str(), prev_str_value.size()} != E.get_value_as_string();
  }

  template<class BASE, int OPERATION>
  const mixed &get_processing_value(const lev_confdata_store_wrapper<BASE, OPERATION> &E) noexcept {
    if (processing_value_.is_null()) {
      processing_value_ = E.get_value_as_var();
    }
    return processing_value_;
  }

  array<mixed> prepare_array_for(vk::string_view key) const noexcept {
    auto size_hint_it = size_hints_.find(key);
    return size_hint_it != size_hints_.end()
           ? array<mixed>{size_hint_it->second}
           : array<mixed>{};
  }

  bool is_key_blacklisted(vk::string_view key) const noexcept {
    return blacklist_enabled_ && key_blacklist_.is_blacklisted(key);
  }

  // TODO: 'now' is used by engines, can we use it in the same way?
  // It looks like "yes" in practice, but it looks weird ¯\_(ツ)_/¯
  static int get_now() noexcept { return now; }

  using GarbageList = std::forward_list<ConfdataGarbageNode>;

  std::aligned_storage_t<sizeof(confdata_sample_storage), alignof(confdata_sample_storage)> confdata_mem_;
  confdata_sample_storage *updating_confdata_storage_{nullptr};
  std::aligned_storage_t<sizeof(GarbageList), alignof(GarbageList)> garbage_mem_;
  GarbageList *garbage_from_previous_confdata_sample_{nullptr};
  size_t garbage_size_{0};
  mixed last_element_in_garbage_;
  bool confdata_has_any_updates_{false};
  std::unordered_map<vk::string_view, array_size> size_hints_;
  ConfdataStats::EventCounters event_counters_;

  ConfdataKeyMaker processing_key_;
  mixed processing_value_;

  std::unordered_map<vk::string_view, int> element_delays_;
  std::multimap<int, std::string> expiration_trace_;

  bool blacklist_enabled_{true};
  const ConfdataKeyBlacklist &key_blacklist_;
  const ConfdataPredefinedWildcards &predefined_wildcards_;
};

struct {
  const char *binlog_mask{nullptr};
  size_t memory_limit{2u * 1024u * 1024u * 1024u};
  std::unique_ptr<re2::RE2> key_blacklist_pattern;
  std::unordered_set<vk::string_view> predefined_wildcards;

  bool is_enabled() const noexcept {
    return binlog_mask;
  }
} confdata_settings;

} // namespace

void set_confdata_binlog_mask(const char *mask) noexcept {
  confdata_settings.binlog_mask = mask;
}

void set_confdata_memory_limit(size_t memory_limit) noexcept {
  confdata_settings.memory_limit = memory_limit;
}

void set_confdata_blacklist_pattern(std::unique_ptr<re2::RE2> &&key_blacklist_pattern) noexcept {
  confdata_settings.key_blacklist_pattern = std::move(key_blacklist_pattern);
}

void add_confdata_predefined_wildcard(const char *wildcard) noexcept {
  assert(wildcard && *wildcard);
  vk::string_view wildcard_value{wildcard};
  confdata_settings.predefined_wildcards.emplace(wildcard_value);
}

void clear_confdata_predefined_wildcards() noexcept {
  confdata_settings.predefined_wildcards.clear();
}

void init_confdata_binlog_reader() noexcept {
  if (!confdata_settings.is_enabled()) {
    return;
  }

  static engine_settings_t settings = {};
  settings.name = NAME_VERSION;
  settings.load_index = []() {
    return ConfdataBinlogReplayer::get().load_index();
  };
  settings.replay_logevent = [](const lev_generic *E, int size) {
    return ConfdataBinlogReplayer::get().replay(E, size);
  };
  settings.on_lev_start = [](const lev_start *E) {
    log_split_min = E->split_min;
    log_split_max = E->split_max;
    log_split_mod = E->split_mod;
  };
  set_engine_settings(&settings);

  vkprintf(1, "start confdata loading\n");

  auto &confdata_stats = ConfdataStats::get();
  confdata_stats.initial_loading_time = -std::chrono::steady_clock::now().time_since_epoch();

  auto &confdata_manager = ConfdataGlobalManager::get();
  confdata_manager.init(confdata_settings.memory_limit,
                        std::move(confdata_settings.predefined_wildcards),
                        std::move(confdata_settings.key_blacklist_pattern));

  dl::set_current_script_allocator(confdata_manager.get_resource(), true);
  // engine_default_load_index and engine_default_read_binlog call exit(1) on errors,
  // which runs all global/static variables destructors.
  // This static variable restores the confdata allocator,
  // so other global/static variables can be freed normally.
  static auto confdata_allocator_rollback = vk::finally([] {
    dl::restore_default_script_allocator(true);
  });

  auto &confdata_binlog_replayer = ConfdataBinlogReplayer::get();
  confdata_binlog_replayer.init(confdata_manager.get_resource());
  engine_default_load_index(confdata_settings.binlog_mask);
  engine_default_read_binlog();
  confdata_binlog_replayer.delete_expired_elements();

  auto loaded_confdata = confdata_binlog_replayer.finish_confdata_update();
  assert(loaded_confdata.previous_confdata_garbage.empty());

  confdata_stats.on_update(loaded_confdata.new_confdata,
                           loaded_confdata.previous_confdata_garbage_size,
                           confdata_manager.get_predefined_wildcards());
  confdata_stats.initial_loading_time += confdata_stats.last_update_time_point.time_since_epoch();

  confdata_manager.get_current().reset(std::move(loaded_confdata.new_confdata));

  vkprintf(1, "confdata loaded\n");
  confdata_allocator_rollback.disable();
  dl::restore_default_script_allocator(true);
}

void confdata_binlog_update_cron() noexcept {
  if (!confdata_settings.is_enabled()) {
    return;
  }

  auto &confdata_stats = ConfdataStats::get();
  confdata_stats.total_updating_time -= std::chrono::steady_clock::now().time_since_epoch();
  auto &confdata_manager = ConfdataGlobalManager::get();
  auto &mem_resource = confdata_manager.get_resource();
  dl::set_current_script_allocator(mem_resource, true);

  auto &previous_confdata_sample = confdata_manager.get_current();
  auto &confdata_binlog_replayer = ConfdataBinlogReplayer::get();
  confdata_binlog_replayer.try_use_previous_confdata_storage_as_init(previous_confdata_sample.get_confdata());

  binlog_try_read_events();
  confdata_binlog_replayer.delete_expired_elements();

  if (confdata_binlog_replayer.has_new_confdata()){
    if (confdata_manager.can_next_be_updated()) {
      auto updated_confdata = confdata_binlog_replayer.finish_confdata_update();
      confdata_stats.on_update(updated_confdata.new_confdata,
                               updated_confdata.previous_confdata_garbage_size,
                               confdata_manager.get_predefined_wildcards());
      previous_confdata_sample.save_garbage(std::move(updated_confdata.previous_confdata_garbage));
      const bool switched = confdata_manager.try_switch_to_next_sample(std::move(updated_confdata.new_confdata));
      assert(switched);
    } else {
      ++confdata_stats.ignored_updates;
    }
  }

  confdata_manager.clear_unused_samples();

  dl::restore_default_script_allocator(true);
  confdata_stats.total_updating_time += std::chrono::steady_clock::now().time_since_epoch();
}

void write_confdata_stats_to(stats_t *stats) noexcept {
  if (confdata_settings.is_enabled()) {
    auto &confdata_stats = ConfdataStats::get();
    auto &binlog_replayer = ConfdataBinlogReplayer::get();
    confdata_stats.elements_with_delay = binlog_replayer.get_elements_with_delay_count();
    confdata_stats.event_counters = binlog_replayer.get_event_counters();
    confdata_stats.write_stats_to(stats, ConfdataGlobalManager::get().get_resource().get_memory_stats());
  }
}
