#include "server/confdata-binlog-replay.h"

#include <bitset>
#include <chrono>
#include <cinttypes>
#include <forward_list>
#include <map>

#include "binlog/binlog-replayer.h"
#include "common/precise-time.h"
#include "common/server/engine-settings.h"
#include "common/server/init-binlog.h"
#include "common/server/init-snapshot.h"
#include "common/wrappers/string_view.h"
#include "kfs/kfs.h"

#include "runtime/allocator.h"
#include "runtime/confdata-global-manager.h"
#include "runtime/kphp_core.h"
#include "server/confdata-binlog-events.h"
#include "server/confdata-stats.h"

namespace {

class ConfdataBinlogReplayer : vk::binlog::replayer {
public:
  enum class OperationStatus {
    no_update,
    ttl_update_only,
    full_update
  };

  static ConfdataBinlogReplayer &get() noexcept {
    static ConfdataBinlogReplayer loader;
    return loader;
  }

  using vk::binlog::replayer::replay;

  size_t try_reserve_for_snapshot(const vk::string_view &key, size_t search_from,
                                  vk::string_view &prev_key, array_size &counter) noexcept {
    auto dot_pos = key.find('.', search_from);
    if (dot_pos != std::string::npos) {
      const auto dot_key = key.substr(0, dot_pos + 1);
      if (prev_key != dot_key) {
        if (counter.int_size + counter.string_size > 1) {
          size_hints_[prev_key] = counter;
        }
        prev_key = dot_key;
        counter = array_size{};
      }
      const auto key_tail = key.substr(dot_pos + 1);
      if (!key_tail.empty() && php_is_int(key_tail.data(), key_tail.size())) {
        ++counter.int_size;
      } else {
        ++counter.string_size;
      }
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
    loading_from_snapshot_ = true;

    vk::string_view last_one_dot_key;
    vk::string_view last_two_dots_key;
    array_size one_dot_elements_counter;
    array_size two_dots_elements_counter;
    for (int i = 0; i < nrecords; i++) {
      const auto &element = reinterpret_cast<const entry_type &>(index_binary_data[index_offset[i]]);
      const vk::string_view key{element.data, static_cast<size_t>(std::max(element.key_len, short{0}))};
      if (key.empty() || is_key_blacklisted(key)) {
        index_offset[i] = -1;
      } else {
        const auto first_dot = try_reserve_for_snapshot(key, 0, last_one_dot_key, one_dot_elements_counter);
        if (first_dot != std::string::npos) {
          try_reserve_for_snapshot(key, first_dot + 1, last_two_dots_key, two_dots_elements_counter);
        }
      }
    }

    auto blacklist = std::move(key_blacklist_pattern_);
    for (int i = 0; i < nrecords; i++) {
      if (index_offset[i] >= 0) {
        store_element(reinterpret_cast<const entry_type &>(index_binary_data[index_offset[i]]));
      }
    }
    key_blacklist_pattern_ = std::move(blacklist);
    loading_from_snapshot_ = false;
    size_hints_.clear();
    return 0;
  }

  void delete_element(const char *key, short key_len) noexcept {
    generic_operation(key, key_len, -1, [this] { return delete_processing_element(); });
  }

  void touch_element(const lev_confdata_touch &E) noexcept {
    generic_operation(E.key, E.key_len, E.delay, [this] { return touch_processing_element(); });
  }

  template<class BASE, int OPERATION>
  void store_element(const lev_confdata_store_wrapper<BASE, OPERATION> &E) noexcept {
    // даже не пытайся захватить E по значению, он попытается её скопировать и будет очень грустно :(
    generic_operation(E.data, E.key_len, E.get_delay(), [this, &E] { return store_processing_element(E); });
  }

  void unsupported_operation(const char *operation_name, const char *key, int key_len) noexcept {
    ++event_counters_.unsupported_total_events;
    kprintf("Confdata binlog reading error: got unsupported operation '%s' with key '%.*s'\n", operation_name, std::max(key_len, 0), key);
  }

  void init(memory_resource::unsynchronized_pool_resource &memory_pool,
            std::unique_ptr<re2::RE2> &&key_blacklist_pattern) noexcept {
    assert(!updating_confdata_storage_);
    updating_confdata_storage_ = new(&confdata_mem_)confdata_sample_storage{confdata_sample_storage::allocator_type{memory_pool}};
    key_blacklist_pattern_ = std::move(key_blacklist_pattern);
  }

  struct ConfdataUpdateResult {
    confdata_sample_storage new_confdata;
    std::forward_list<ConfdataGarbageNode> previous_confdata_garbage;
    size_t previous_confdata_garbage_size;
  };

  ConfdataUpdateResult finish_confdata_update() noexcept {
    for (auto &confdata_section: *updating_confdata_storage_) {
      // сохраняем в отдельную переменную, что бы не делать const_cast
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
      std::move(garbage_from_previous_confdata_sample_),
      garbage_size_
    };
    // после move конейнер остется в "a valid but unspecified state", поэтмому лучше сделать ему явный clear
    updating_confdata_storage_->clear();
    garbage_from_previous_confdata_sample_.clear();
    garbage_size_ = 0;
    confdata_has_any_updates_ = false;
    return result;
  }

  void try_use_previous_confdata_storage_as_init(const confdata_sample_storage &previous_confdata_storage) noexcept {
    if (!confdata_has_any_updates_) {
      assert(garbage_from_previous_confdata_sample_.empty());
      if (updating_confdata_storage_->empty()) {
        *updating_confdata_storage_ = previous_confdata_storage;
      } else {
        // вообще говоря, они должны быть идентичны, но это сложно/затратно проверять
        assert(updating_confdata_storage_->size() == previous_confdata_storage.size());
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
  ConfdataBinlogReplayer() noexcept {
    add_handler([this](const lev_confdata_delete &E) {
      ++event_counters_.delete_events;
      this->delete_element(E.key, E.key_len);
    });
    add_handler([this](const lev_confdata_touch &E) {
      ++event_counters_.touch_events;
      this->touch_element(E);
    });

    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_add> &E) {
      ++event_counters_.add_events;
      this->store_element(E);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_set> &E) {
      ++event_counters_.set_events;
      this->store_element(E);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store, pmct_replace> &E) {
      ++event_counters_.replace_events;
      this->store_element(E);
    });

    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_add> &E) {
      ++event_counters_.add_forever_events;
      this->store_element(E);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_set> &E) {
      ++event_counters_.set_forever_events;
      this->store_element(E);
    });
    add_handler([this](const lev_confdata_store_wrapper<lev_pmemcached_store_forever, pmct_replace> &E) {
      ++event_counters_.replace_forever_events;
      this->store_element(E);
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
  void generic_operation(const char *key, short key_len, int delay, const F &operation) noexcept {
    // TODO ассерт?
    if (key_len < 0) {
      return;
    }
    const vk::string_view key_view{key, static_cast<size_t>(key_len)};
    if (is_key_blacklisted(key_view)) {
      return;
    }

    assert(last_element_in_garbage_.is_null());
    assert(processing_value_.is_null());
    static_assert(std::is_same<short, int16_t>{}, "short is expected to be int16_t");
    const auto first_key_dots = processing_key_.update(key, key_len);
    const auto operation_status = operation();
    if (operation_status == OperationStatus::no_update) {
      assert(last_element_in_garbage_.is_null());
      assert(processing_value_.is_null());
      return;
    }
    if (operation_status == OperationStatus::full_update && first_key_dots == FirstKeyDots::two) {
      processing_key_.forcibly_change_first_key_dots_from_two_to_one();
      const auto should_be_full = operation();
      assert(should_be_full == OperationStatus::full_update);
    }

    last_element_in_garbage_.clear();
    processing_value_.clear();
    if (operation_status == OperationStatus::full_update) {
      confdata_has_any_updates_ = true;
    }
    update_element_in_expiration_trace(key_view, delay);
  }

  confdata_sample_storage::iterator find_updating_confdata_first_key_element() {
    // это некоторая оптимизация, позволяющая ускорить загрузку снепшота
    if (loading_from_snapshot_ && processing_key_.get_first_key_dots() != FirstKeyDots::zero) {
      auto last_it = updating_confdata_storage_->end();
      size_t attempts = std::min(2ul, updating_confdata_storage_->size());
      while (attempts--) {
        if ((--last_it)->first == processing_key_.get_first_key()) {
          return last_it;
        }
      }
    }
    return updating_confdata_storage_->find(processing_key_.get_first_key());
  }

  OperationStatus delete_processing_element() noexcept {
    auto first_key_it = updating_confdata_storage_->find(processing_key_.get_first_key());
    if (first_key_it == updating_confdata_storage_->end()) {
      return OperationStatus::no_update;
    }

    // для ключей без '.'
    if (processing_key_.get_first_key_dots() == FirstKeyDots::zero) {
      // убираем в мусор что было
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

    // убираем в мусор что было, далее будет копирование с расщеплением
    put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);

    array_for_second_key.mutate_if_shared();
    auto second_key_it = array_for_second_key.find_no_mutate(processing_key_.get_second_key());
    assert(second_key_it != array_for_second_key.end());
    // массив должен быть расщиплен
    assert(array_for_second_key.get_reference_counter() == 1);
    // ключ и значение убираем в мусор
    if (second_key_it.is_string_key() && second_key_it.get_string_key().get_reference_counter() != 1) {
      // ключ убираем с deep_last, потому что он дожен быть уничтожен строго после array_for_second_key
      put_confdata_var_into_garbage(second_key_it.get_string_key(), ConfdataGarbageDestroyWay::deep_last);
    }
    put_confdata_element_value_into_garbage(second_key_it.get_value());
    array_for_second_key.unset(processing_key_.get_second_key());
    if (array_for_second_key.empty()) {
      // имя убираем в мусор, сама же секция удалится, так как была расщеплена (см. выше)
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

    // для ключей без '.'
    if (processing_key_.get_first_key_dots() == FirstKeyDots::zero) {
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
    auto first_key_it = find_updating_confdata_first_key_element();
    bool element_exists = true;
    if (first_key_it == updating_confdata_storage_->end()) {
      element_exists = false;
      // при загрузке снепшота, все ключи отсортированы, поэтому имеет смысл вставлять в конец
      first_key_it = updating_confdata_storage_->emplace_hint(first_key_it, processing_key_.make_first_key_copy(), var{});
    }

    // для ключей без '.'
    if (processing_key_.get_first_key_dots() == FirstKeyDots::zero) {
      if (!can_element_be_saved(E, element_exists)) {
        return OperationStatus::no_update;
      }

      if (is_new_value(E, first_key_it->second)) {
        // убираем в мусор предыдущее значение и перезаписываем новым
        put_confdata_element_value_into_garbage(first_key_it->second);
        first_key_it->second = get_processing_value(E);
        return OperationStatus::full_update;
      }
      return OperationStatus::ttl_update_only;
    }

    // по дефолту вставляется null
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
      // убираем в мусор что было, далее будет копирование с расщеплением
      put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);
      array_for_second_key.set_value(processing_key_.make_second_key_copy(), get_processing_value(E));
      // вставка элемента расщепит массив
      assert(array_for_second_key.get_reference_counter() == 1);
      return OperationStatus::full_update;
    }
    if (is_new_value(E, *prev_value)) {
      // убираем в мусор что было, далее будет копирование с расщеплением
      put_confdata_var_into_garbage(array_for_second_key, ConfdataGarbageDestroyWay::shallow_first);
      array_for_second_key.mutate_if_shared();
      auto second_key_it = array_for_second_key.find_no_mutate(processing_key_.get_second_key());
      assert(second_key_it != array_for_second_key.end());
      // массив должен быть расщиплен
      assert(array_for_second_key.get_reference_counter() == 1);
      // предыдущее значение убираем в мусор и сохраняем новое
      put_confdata_element_value_into_garbage(second_key_it.get_value());
      second_key_it.get_value() = get_processing_value(E);
      return OperationStatus::full_update;
    }
    return OperationStatus::ttl_update_only;
  }

  void put_confdata_element_value_into_garbage(const var &element) noexcept {
    if (!last_element_in_garbage_.is_null()) {
      // так как для ключей с 1 и 2 точками мы можем использовать один и тот же эелемнет,
      // то и тут они внутренние указатели должны совпадать
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
        vk::none_of_equal(element.get_reference_counter(), 1, 2)) {
      put_confdata_var_into_garbage(element, ConfdataGarbageDestroyWay::deep_last);
      last_element_in_garbage_ = element;
    }
  }

  template<typename T>
  void put_confdata_var_into_garbage(const T &v, ConfdataGarbageDestroyWay destroy_way) noexcept {
    if (v.is_reference_counter(ExtraRefCnt::for_confdata)) {
      garbage_from_previous_confdata_sample_.emplace_front(ConfdataGarbageNode{v, destroy_way});
      ++garbage_size_;
      return;
    }
    if (v.is_reference_counter(ExtraRefCnt::for_global_const) ||
        (destroy_way == ConfdataGarbageDestroyWay::shallow_first && v.get_reference_counter() == 1)) {
      return;
    }
    assert(!"Got unexpected reference counter");
  }

  static void mark_string_as_confdata_const(string &str) noexcept {
    if (str.is_reference_counter(ExtraRefCnt::for_confdata) ||
        str.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return;
    }
    assert(vk::any_of_equal(str.get_reference_counter(), 1, 2));
    str.set_reference_counter_to(ExtraRefCnt::for_confdata);
  }

  static void mark_array_as_confdata_const(array<var> &arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_confdata) ||
        arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return;
    }
    assert(vk::any_of_equal(arr.get_reference_counter(), 1, 2));
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
      // если элемент не существует, мы можем сделать pmct_add, но не можем сделать pmct_replace
      return OPERATION == pmct_add;
    }
    // элемент существует, проверим не протух ли он
    vk::string_view key{E.data, static_cast<size_t>(E.key_len)};
    auto it = element_delays_.find(key);
    // если элемента нет в element_delays_, значит он точно не протух
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
  bool is_new_value(const lev_confdata_store_wrapper<BASE, OPERATION> &E, const var &prev_value) noexcept {
    if (E.get_flags()) {
      return !equals(get_processing_value(E), prev_value);
    }
    if (!prev_value.is_string()) {
      return true;
    }
    const auto &prev_str_value = prev_value.as_string();
    return vk::string_view{prev_str_value.c_str(), prev_str_value.size()} != E.get_value_as_string();
  }

  template<class BASE, int OPERATION>
  const var &get_processing_value(const lev_confdata_store_wrapper<BASE, OPERATION> &E) noexcept {
    if (processing_value_.is_null()) {
      processing_value_ = E.get_value_as_var();
    }
    return processing_value_;
  }

  array<var> prepare_array_for(vk::string_view key) const noexcept {
    auto size_hint_it = size_hints_.find(key);
    return size_hint_it != size_hints_.end()
           ? array<var>{size_hint_it->second}
           : array<var>{};
  }

  bool is_key_blacklisted(vk::string_view key) const noexcept {
    return key_blacklist_pattern_ &&
           re2::RE2::FullMatch(re2::StringPiece(key.data(), key.size()), *key_blacklist_pattern_);
  }

  // TODO: 'now' используется движками, можем ли мы так же её использовать?
  // На парктике звучит как да, но выглядит не очень ¯\_(ツ)_/¯
  static int get_now() noexcept { return now; }

  std::aligned_storage_t<sizeof(confdata_sample_storage), alignof(confdata_sample_storage)> confdata_mem_;
  confdata_sample_storage *updating_confdata_storage_{nullptr};
  std::forward_list<ConfdataGarbageNode> garbage_from_previous_confdata_sample_;
  size_t garbage_size_{0};
  var last_element_in_garbage_;
  bool confdata_has_any_updates_{false};
  bool loading_from_snapshot_{false};
  std::unordered_map<vk::string_view, array_size> size_hints_;
  ConfdataStats::EventCounters event_counters_;

  ConfdataKeyMaker processing_key_;
  var processing_value_;

  std::unordered_map<vk::string_view, int> element_delays_;
  std::multimap<int, std::string> expiration_trace_;

  std::unique_ptr<re2::RE2> key_blacklist_pattern_;
};

struct {
  const char *binlog_mask{nullptr};
  dl::size_type memory_limit{2u * 1024u * 1024u * 1024u};
  std::unique_ptr<re2::RE2> key_blacklist_pattern;

  bool is_enabled() const noexcept {
    return binlog_mask;
  }
} confdata_settings;

} // namespace

void set_confdata_binlog_mask(const char *mask) noexcept {
  confdata_settings.binlog_mask = mask;
}

void set_confdata_memory_limit(dl::size_type memory_limit) noexcept {
  confdata_settings.memory_limit = memory_limit;
}

void set_confdata_blacklist_pattern(std::unique_ptr<re2::RE2> &&key_blacklist_pattern) noexcept {
  confdata_settings.key_blacklist_pattern = std::move(key_blacklist_pattern);
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
  confdata_manager.init(confdata_settings.memory_limit);

  dl::set_current_script_allocator(confdata_manager.get_resource(), true);
  // engine_default_load_index и engine_default_read_binlog в случае проблем делают exit(1),
  // что приводит к вызову всех деструкторов глобальных и статических переменных.
  // Эта статическая переменная откатывает аллокатор конфдаты,
  // чтобы другие глобальные и статические переменные могли нормально удалиться
  static auto confdata_allocator_rollback = vk::finally([] {
    dl::restore_default_script_allocator(true);
  });

  auto &confdata_binlog_replayer = ConfdataBinlogReplayer::get();
  confdata_binlog_replayer.init(confdata_manager.get_resource(),
                                std::move(confdata_settings.key_blacklist_pattern));
  engine_default_load_index(confdata_settings.binlog_mask);
  engine_default_read_binlog();
  confdata_binlog_replayer.delete_expired_elements();

  auto loaded_confdata = confdata_binlog_replayer.finish_confdata_update();
  assert(loaded_confdata.previous_confdata_garbage.empty());

  confdata_stats.on_update(loaded_confdata.new_confdata, loaded_confdata.previous_confdata_garbage_size);
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
      confdata_stats.on_update(updated_confdata.new_confdata, updated_confdata.previous_confdata_garbage_size);
      previous_confdata_sample.save_garbage(std::move(updated_confdata.previous_confdata_garbage));
      const bool switched = confdata_manager.try_switch_to_next_sample(std::move(updated_confdata.new_confdata));
      assert(switched);
    } else {
      ++confdata_stats.ignored_updates;
    }
  }

  // TODO подумать над дефрагментацией!
  // Делать дефрагментацию каждый раз - плохо, необходимо подумать о условиях, когда её реально лучше всего делать.
  // Либо вообще забить и никогда не делать, а комментарий удалить.
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
