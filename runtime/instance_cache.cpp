#include "runtime/instance_cache.h"

#include <chrono>
#include <forward_list>
#include <map>
#include <mutex>
#include <sys/mman.h>
#include <unordered_set>

#include "common/kprintf.h"
#include "common/parallel/lock_accessible.h"
#include "common/smart_ptrs/not_owner_ptr.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/inter_process_mutex.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/memory_resource/synchronized_pool_resource.h"
#include "server/php-engine-vars.h"

namespace ic_impl_ {

//#define DEBUG_INSTANCE_CACHE

#ifdef DEBUG_INSTANCE_CACHE
  #define ic_debug kprintf
#else
inline void ic_debug(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
inline void ic_debug(const char *, ...) {}
#endif

class AllocReplacementSection : dl::CriticalSectionGuard {
public:
  enum Mode {
    NORMAL,
    FORBID_ALLOCATIONS
  };

  explicit AllocReplacementSection(memory_resource::synchronized_pool_resource &resource, Mode mode = NORMAL) :
    resource_(resource),
    mode_(mode) {
    php_assert(!dl::replace_malloc_with_script_allocator);
    dl::set_script_allocator_replacement(&resource_);
    if (mode_ == FORBID_ALLOCATIONS) {
      resource_.forbid_allocations();
    }
  }

  ~AllocReplacementSection() {
    if (mode_ == FORBID_ALLOCATIONS) {
      resource_.allow_allocations();
    }
    dl::drop_script_allocator_replacement();
    php_assert(!dl::replace_malloc_with_script_allocator);
  }

private:
  memory_resource::synchronized_pool_resource &resource_;
  Mode mode_;
};

class ElementHolder : private vk::thread_safe_refcnt<ElementHolder> {
private:
  ElementHolder(std::chrono::nanoseconds now, int ttl,
                std::unique_ptr<InstanceWrapperBase> &&instance,
                memory_resource::synchronized_pool_resource &mem_resource,
                InstanceCacheStats &stats) :
    inserted_by_process(getpid()),
    instance_wrapper(std::move(instance)),
    resource(mem_resource),
    cache_stats(stats) {
    update_time_points(now, ttl);
    ++cache_stats.elements_created;
  }

public:
  using vk::thread_safe_refcnt<ElementHolder>::add_ref;
  using vk::thread_safe_refcnt<ElementHolder>::get_refcnt;

  void release() {
    if (--refcnt == 0) {
      ++cache_stats.elements_destroyed;
      auto &mem_resource = resource;
      this->~ElementHolder();
      mem_resource.deallocate(this, sizeof(ElementHolder));
    }
  }

  static vk::intrusive_ptr<ElementHolder> construct(std::chrono::nanoseconds now, int ttl,
                                                    std::unique_ptr<InstanceWrapperBase> &&instance,
                                                    memory_resource::synchronized_pool_resource &resource,
                                                    InstanceCacheStats &stats) {
    if (!instance || !resource.reserve(sizeof(ElementHolder))) {
      return {};
    }
    void *mem = resource.allocate(sizeof(ElementHolder));
    php_assert(mem);
    return vk::intrusive_ptr<ElementHolder>{new(mem) ElementHolder{now, ttl, std::move(instance), resource, stats}};
  }

  // check that left less than (total_time * ratio) time
  bool is_left_less_than(std::chrono::nanoseconds now, double ratio) const {
    return now > ratio * std::chrono::duration<double>{stored_at} - (ratio - 1) * std::chrono::duration<double>{expiring_at};
  }

  void update_time_points(std::chrono::nanoseconds now, int ttl) {
    stored_at = std::max(now, stored_at);
    expiring_at = ttl > 0 ? stored_at + std::chrono::seconds{ttl} : std::chrono::nanoseconds::max();
    try_return_null_early = true;
  }

  std::chrono::nanoseconds stored_at{std::chrono::nanoseconds::min()};
  std::chrono::nanoseconds expiring_at{std::chrono::nanoseconds::max()};
  const pid_t inserted_by_process{0};
  bool try_return_null_early{true};
  std::unique_ptr<InstanceWrapperBase> instance_wrapper;
  memory_resource::synchronized_pool_resource &resource;
  InstanceCacheStats &cache_stats;
};

template<typename T>
using InterProcessAllocator_ = memory_resource::resource_allocator<T, memory_resource::synchronized_pool_resource>;
using ElementStorage_ = std::map<string, vk::intrusive_ptr<ElementHolder>, std::less<string>,
  InterProcessAllocator_<std::pair<const string, vk::intrusive_ptr<ElementHolder>>>>;
using ProcessingKeys_ = std::map<string, std::chrono::nanoseconds, std::less<string>,
  InterProcessAllocator_<std::pair<const string, std::chrono::nanoseconds>>>;
using ExpirationTrace_ = std::multimap<std::chrono::nanoseconds, string, std::less<std::chrono::nanoseconds>,
  InterProcessAllocator_<std::pair<const std::chrono::nanoseconds, string>>>;

struct SharedDataStorages {
  explicit SharedDataStorages(memory_resource::synchronized_pool_resource &resource) :
    storage(ElementStorage_::allocator_type{resource}),
    expiration_trace(ExpirationTrace_::allocator_type{resource}),
    processing_keys(ProcessingKeys_::allocator_type{resource}) {
  }

  inter_process_mutex mutex;
  ElementStorage_ storage;
  ExpirationTrace_ expiration_trace;
  ProcessingKeys_ processing_keys;
  InstanceCacheStats stats;
};

class SharedMemoryData : vk::not_copyable {
public:
  void init(dl::size_type pool_size) {
    php_assert(!data);
    memory_resource.init(pool_size);
    construct_data_inplace();
  }

  void hard_reset() {
    php_assert(data);
    php_assert(data->processing_keys.empty());
    // to avoid of calling all destructors, just free mutex
    data->mutex.~inter_process_mutex();
    memory_resource.hard_reset();
    construct_data_inplace();
  }

  void free() {
    php_assert(data);
    php_assert(data->processing_keys.empty());
    // to avoid of calling all destructors, just free mutex
    data->mutex.~inter_process_mutex();
    data = nullptr;
    memory_resource.free();
  }

  memory_resource::synchronized_pool_resource memory_resource;
  SharedDataStorages *data{nullptr};

private:
  void construct_data_inplace() {
    if (!memory_resource.reserve(sizeof(SharedDataStorages))) {
      php_critical_error("not enough memory to continue");
    }
    void *mem = memory_resource.allocate(sizeof(SharedDataStorages));
    php_assert(mem);
    if (data) {
      // in case of hard reset, data should be allocated on same place
      php_assert(data == mem);
    }
    data = new(mem) SharedDataStorages{memory_resource};
  }
};

class ControlBlock {
public:
  uint32_t acquire_active_memory_id(pid_t user_pid, size_t user_index) {
    const uint32_t memory_id = get_active_memory_id();
    get_user_pid_ref(memory_id, user_index) = user_pid;
    return memory_id;
  }

  void release(uint32_t memory_id, pid_t user_pid, size_t user_index) {
    pid_t &stored_pid = get_user_pid_ref(memory_id, user_index);
    php_assert(stored_pid == user_pid);
    stored_pid = 0;
  }

  uint32_t get_active_memory_id() const {
    return active_memory_id_;
  }

  uint32_t get_inactive_memory_id() const {
    return (active_memory_id_ + 1) % 2;
  }

  bool is_inactive_unused(const pid_t *active_workers_begin, const pid_t *active_workers_end) {
    const uint32_t inactive_memory_id = get_inactive_memory_id();
    size_t user_index = 0;
    for (auto active_worker = active_workers_begin; active_worker != active_workers_end; ++active_worker) {
      pid_t &stored_pid = get_user_pid_ref(inactive_memory_id, user_index);
      if (*active_worker && *active_worker == stored_pid) {
        return false;
      }
      stored_pid = 0;
      ++user_index;
    }
    return true;
  }

  void swap_active() {
    active_memory_id_ = get_inactive_memory_id();
  }

private:
  pid_t &get_user_pid_ref(uint32_t memory_id, size_t user_index) {
    php_assert(memory_id < 2);
    php_assert(users_.size() > 2 * user_index + memory_id);
    // memory_id == 0 => user id is even
    // memory_id == 1 => user id is odd
    return users_[2 * user_index + memory_id];
  }

  uint32_t active_memory_id_{0};
  std::array<pid_t, 2*MAX_WORKERS> users_{{0}};
};

class SharedMemoryDataManager {
public:
  void init(dl::size_type pool_size) {
    for (auto &mem: swappable_memory_) {
      mem.init(pool_size);
    }
    void *mem = mmap(nullptr, sizeof(vk::lock_accessible<ControlBlock, inter_process_mutex>),
                     PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    php_assert(mem);
    control_block_ = new(mem) vk::lock_accessible<ControlBlock, inter_process_mutex>{};
  }

  SharedMemoryData *acquire_current_memory_data(int user_worker_id) {
    php_assert(user_worker_id >= 0);
    php_assert(control_block_);
    const uint32_t memory_id = (*control_block_)->acquire_active_memory_id(getpid(), static_cast<size_t>(user_worker_id));
    return &swappable_memory_[memory_id];
  }

  void release_memory_data(int user_worker_id, SharedMemoryData *data) {
    php_assert(user_worker_id >= 0);
    php_assert(control_block_);
    const uint32_t memory_id = (data == &swappable_memory_.front()) ? 0 : 1;
    (*control_block_)->release(memory_id, getpid(), static_cast<size_t>(user_worker_id));
  }

  // this function should be called only from master
  SharedMemoryData &get_current_memory_data() {
    php_assert(control_block_);
    return swappable_memory_[(*control_block_)->get_active_memory_id()];
  }

  // this function should be called only from master
  bool try_swap_if_inactive(const pid_t *active_workers_begin, const pid_t *active_workers_end) {
    php_assert(control_block_);
    if((*control_block_)->is_inactive_unused(active_workers_begin, active_workers_end)) {
      swappable_memory_[(*control_block_)->get_inactive_memory_id()].hard_reset();
      (*control_block_)->swap_active();
      return true;
    }
    return false;
  }

  void free() {
    php_assert(control_block_);
    control_block_->~lock_accessible();
    munmap(control_block_, sizeof(std::atomic<SharedMemoryData *>));
    control_block_ = nullptr;

    for (auto &mem: swappable_memory_) {
      mem.free();
    }
  }

private:
  std::array<SharedMemoryData, 2> swappable_memory_;
  vk::lock_accessible<ControlBlock, inter_process_mutex> *control_block_{nullptr};
};

class InstanceCache {
private:
  static constexpr size_t DEFAULT_MEMORY_LIMIT{256u * 1024u * 1024u};
  static constexpr double MEMORY_USED_THRESHOLD{0.9};
  static constexpr double REAL_MEMORY_USED_THRESHOLD{0.9};
  static constexpr double FRESHNESS_ELEMENT_RATIO{0.8};
  static constexpr double EARLY_EXPIRATION_ELEMENT_RATIO{0.2};
  static constexpr std::chrono::seconds DEAD_PROCESSING_WORKER_TIMEOUT{3};
  static constexpr std::chrono::minutes PHYSICAL_REMOVING_DELAY{1};

  InstanceCache() :
    now_(std::chrono::system_clock::now().time_since_epoch()) {
  }

public:
  static InstanceCache &get() {
    static InstanceCache cache;
    return cache;
  }

  void global_init() {
    php_assert(!current_);
    initial_process_ = getpid();
    data_manager_.init(total_memory_limit_);
  }

  void refresh(int user_worker_id) {
    php_assert(!current_);
    update_now();
    current_ = data_manager_.acquire_current_memory_data(user_worker_id);
  }

  void update_now() {
    now_ = std::chrono::system_clock::now().time_since_epoch();
  }

  void free(int user_worker_id) {
    php_assert(current_);
    // request_cache_ placed in script memory
    request_cache_.clear();
    {
      AllocReplacementSection section{current_->memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
      // used_elements is placed on heap memory
      used_elements_.clear();
    }
    data_manager_.release_memory_data(user_worker_id, current_);
    current_ = nullptr;
  }

  bool store(const string &key, const InstanceWrapperBase &instance_wrapper, int ttl) {
    ic_debug("store '%s'\n", key.c_str());
    php_assert(current_);
    const auto memory_stats = current_->memory_resource.get_memory_stats();
    if (memory_stats.memory_used >= MEMORY_USED_THRESHOLD * memory_stats.memory_limit) {
      instance_wrapper.memory_limit_warning();
      return false;
    }

    // request_cache_ placed in script memory
    // get element here, because lock enable critical section,
    // and if we get out of script memory in critical section, and it may lead crash
    auto &cached_element_ptr = request_cache_[key];
    update_now();
    std::unique_lock<inter_process_mutex> shared_data_lock{current_->data->mutex};
    {
      AllocReplacementSection section{current_->memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
      if (is_element_insertion_can_be_skipped(key)) {
        return false;
      }
    }
    AllocReplacementSection section{current_->memory_resource};
    string key_in_shared_memory = key;
    if (!DeepMoveFromScriptToCacheVisitor{}.process(key_in_shared_memory)) {
      instance_wrapper.memory_limit_warning();
      return false;
    }

    if (start_key_processing(key_in_shared_memory)) {
      shared_data_lock.unlock();
      // move instance from script memory into cache memory
      auto cached_instance_wrapper = instance_wrapper.clone_and_detach_shared_ref();
      auto element = ElementHolder::construct(now_, ttl, std::move(cached_instance_wrapper), current_->memory_resource, current_->data->stats);
      shared_data_lock.lock();
      finish_key_processing(key_in_shared_memory);
      auto inserted_element = insert_element(std::move(key_in_shared_memory), std::move(element));
      cached_element_ptr.reset(inserted_element.get());
      if (inserted_element) {
        ic_debug("element '%s' was successfully inserted\n", key.c_str());
        ++current_->data->stats.elements_stored;
        return true;
      }
    } else {
      DeepDestroyFromCacheVisitor{}.process(key_in_shared_memory);
    }
    instance_wrapper.memory_limit_warning();
    return false;
  }

  InstanceWrapperBase *fetch(const string &key, bool even_if_expired) {
    php_assert(current_);
    // request_cache_ placed in script memory
    if (auto cached_element_ptr = request_cache_.get_value(key)) {
      ic_debug("fetch '%s' from request cache\n", key.c_str());
      return cached_element_ptr->instance_wrapper.get();
    }
    vk::intrusive_ptr<ElementHolder> element;
    bool element_logically_expired = false;
    {
      AllocReplacementSection section{current_->memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
      std::lock_guard<inter_process_mutex> shared_data_lock{current_->data->mutex};
      auto it = current_->data->storage.find(key);
      if (it == current_->data->storage.end()) {
        ic_debug("can't fetch '%s' because it is absent\n", key.c_str());
        ++current_->data->stats.elements_missed;
        return nullptr;
      }

      update_now();
      // if left less than EARLY_EXPIRATION_ELEMENT_RATIO of total time, return null for any process,
      // so that it starts to update data before other processes
      if (it->second->try_return_null_early &&
          it->second->is_left_less_than(now_, EARLY_EXPIRATION_ELEMENT_RATIO)) {
        it->second->try_return_null_early = false;
        ++current_->data->stats.elements_missed_earlier;
        ic_debug("can't fetch '%s' because less than %f of total time is left\n",
                 key.c_str(), EARLY_EXPIRATION_ELEMENT_RATIO);
        return nullptr;
      }
      element_logically_expired = it->second->expiring_at <= now_;
      if (element_logically_expired) {
        if (even_if_expired) {
          ++current_->data->stats.elements_logically_expired_but_fetched;
          ic_debug("fetch logically expired element '%s'\n", key.c_str());
        } else {
          ++current_->data->stats.elements_logically_expired_and_ignored;
          ic_debug("can't fetch '%s' because element was logically expired\n", key.c_str());
          return nullptr;
        }
      } else {
        ++current_->data->stats.elements_fetched;
        ic_debug("fetch '%s' from inter process cache\n", key.c_str());
      }

      element = it->second;
    }

    // used_elements_ is placed on heap memory
    // keep ptr until the end of request
    used_elements_.emplace(element);
    // do not cache logically expired elements
    if (!element_logically_expired) {
      // request_cache_ placed in script memory
      request_cache_.set_value(key, vk::not_owner_ptr<ElementHolder>{element.get()});
    }
    return element->instance_wrapper.get();
  }

  bool update_ttl(const string &key, int ttl) {
    php_assert(current_);
    ic_debug("update_ttl '%s', new ttl '%d'\n", key.c_str(), ttl);
    AllocReplacementSection section{current_->memory_resource};
    std::lock_guard<inter_process_mutex> shared_data_lock{current_->data->mutex};
    auto it = current_->data->storage.find(key);
    if (it == current_->data->storage.end()) {
      return false;
    }

    detach_expiration_trace(it);
    update_now();
    it->second->update_time_points(now_, ttl);
    if (!attach_expiration_trace(it)) {
      // graceful recovery is difficult and useless, just remove the element
      it->second->instance_wrapper->memory_limit_warning();
      remove_element(it);
      return false;
    }
    return true;
  }

  bool del(const string &key) {
    php_assert(current_);
    ic_debug("delete '%s'\n", key.c_str());
    // request_cache_ placed in script memory
    request_cache_.unset(key);
    AllocReplacementSection section{current_->memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
    std::lock_guard<inter_process_mutex> shared_data_lock{current_->data->mutex};
    auto it = current_->data->storage.find(key);
    if (it == current_->data->storage.end()) {
      return false;
    }
    detach_expiration_trace(it);
    remove_element(it);
    return true;
  }

  void clear() {
    php_assert(current_);
    ic_debug("clear\n");
    // request_cache_ placed in script memory
    request_cache_.clear();
    AllocReplacementSection section{current_->memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
    std::lock_guard<inter_process_mutex> shared_data_lock{current_->data->mutex};
    for (auto it = current_->data->storage.begin(); it != current_->data->storage.end();) {
      detach_expiration_trace(it);
      it = remove_element(it);
    }
    php_assert(current_->data->storage.empty());
    php_assert(current_->data->expiration_trace.empty());
  }

  // this function should be called only from master
  void purge_expired() {
    php_assert(initial_process_ == getpid());
    update_now();
    const auto now_with_delay = now_ - PHYSICAL_REMOVING_DELAY;
    auto &current_data = data_manager_.get_current_memory_data();
    AllocReplacementSection section{current_data.memory_resource, AllocReplacementSection::FORBID_ALLOCATIONS};
    std::lock_guard<inter_process_mutex> shared_data_lock{current_data.data->mutex};
    ic_debug("purge expired [cached: %zu, in expiration trace: %zu]\n",
             current_data.data->storage.size(), current_data.data->expiration_trace.size());
    for (auto it = current_data.data->expiration_trace.begin(); it != current_data.data->expiration_trace.end() && it->first <= now_with_delay;) {
      auto storage_it = current_data.data->storage.find(it->second);
      php_assert(storage_it != current_data.data->storage.end());
      php_assert(storage_it->second->expiring_at == it->first);

      ic_debug("purge '%s'\n", it->second.c_str());
      current_data.data->storage.erase(storage_it);
      DeepDestroyFromCacheVisitor{}.process(it->second);
      it = current_data.data->expiration_trace.erase(it);
      ++current_data.data->stats.elements_expired;
    }
  }

  // this function should be called only from master
  void set_memory_limit(dl::size_type limit) {
    total_memory_limit_ = limit;
  }

  bool memory_reserve(dl::size_type size) {
    php_assert(current_);
    return current_->memory_resource.reserve(size);
  }

  void rollback_memory_reserve() {
    php_assert(current_);
    current_->memory_resource.rollback_reserve();
  }

  // this function should be called only from master
  bool is_memory_swap_required() {
    php_assert(initial_process_ == getpid());
    auto &memory_resource = data_manager_.get_current_memory_data().memory_resource;
    if (memory_resource.is_reset_required()) {
      return true;
    }
    const auto memory_stats = memory_resource.get_memory_stats();
    return memory_stats.real_memory_used >= REAL_MEMORY_USED_THRESHOLD * memory_stats.memory_limit;
  }

  // this function should be called only from master
  memory_resource::MemoryStats get_current_memory_stats() {
    php_assert(initial_process_ == getpid());
    return data_manager_.get_current_memory_data().memory_resource.get_memory_stats();
  }

  // this function should be called only from master
  bool try_swap_memory_resource(const pid_t *active_workers_begin, const pid_t *active_workers_end) {
    php_assert(initial_process_ == getpid());
    return data_manager_.try_swap_if_inactive(active_workers_begin, active_workers_end);
  }

  ~InstanceCache() {
    if (initial_process_ != getpid()) {
      return;
    }

    data_manager_.free();
  }

  // this function should be called only from master
  InstanceCacheStats get_stats() {
    auto &current_data = data_manager_.get_current_memory_data();
    std::lock_guard<inter_process_mutex> shared_data_lock{current_data.data->mutex};
    InstanceCacheStats result = current_data.data->stats;
    result.elements_cached = current_data.data->storage.size();
    return result;
  }

private:
  bool is_element_insertion_can_be_skipped(const string &key) const {
    auto it = current_->data->storage.find(key);
    // element can be skipped:
    // 1) if it has been recently inserted from other process
    if (it != current_->data->storage.end() &&
        it->second->expiring_at != std::chrono::nanoseconds::max() &&
        !it->second->is_left_less_than(now_, FRESHNESS_ELEMENT_RATIO) &&
        it->second->inserted_by_process != getpid()) {
      ic_debug("skip '%s' because it was recently updated\n", key.c_str());
      ++current_->data->stats.elements_storing_skipped_due_recent_update;
      return true;
    }
    // 2) if it is being processed right now
    auto processing_key_it = current_->data->processing_keys.find(key);
    if (processing_key_it != current_->data->processing_keys.end()) {
      // processing worker may die and doesn't remove key
      if (now_ - processing_key_it->second < DEAD_PROCESSING_WORKER_TIMEOUT) {
        ic_debug("skip '%s' because it is being processed now\n", key.c_str());
        ++current_->data->stats.elements_storing_skipped_due_processing;
        return true;
      }
      ic_debug("override '%s', looks like previous processing worker has died\n", key.c_str());
    }
    return false;
  }

  vk::intrusive_ptr<ElementHolder> insert_element(string key_in_shared_memory, vk::intrusive_ptr<ElementHolder> element) {
    if (!element) {
      DeepDestroyFromCacheVisitor{}.process(key_in_shared_memory);
      return {};
    }
    auto it = current_->data->storage.find(key_in_shared_memory);
    if (it == current_->data->storage.end()) {
      if (!memory_reserve(ElementStorage_::allocator_type::max_value_type_size())) {
        DeepDestroyFromCacheVisitor{}.process(key_in_shared_memory);
        return {};
      }
      it = current_->data->storage.emplace(std::move(key_in_shared_memory), vk::intrusive_ptr<ElementHolder>{}).first;
    } else {
      detach_expiration_trace(it);
      DeepDestroyFromCacheVisitor{}.process(key_in_shared_memory);
    }
    // perform swap and saving into used_elements_ in order to call element destructor out of shared_data_lock scope
    it->second.swap(element);
    if (element) {
      used_elements_.emplace(std::move(element));
    }
    if (!attach_expiration_trace(it)) {
      // graceful recovery is difficult and useless, just remove the element
      remove_element(it);
      return {};
    }
    used_elements_.emplace(it->second);
    return it->second;
  }

  bool start_key_processing(const string &key) {
    ic_debug("start processing '%s'\n", key.c_str());

    auto processing_key_it = current_->data->processing_keys.find(key);
    if (processing_key_it != current_->data->processing_keys.end()) {
      processing_key_it->second = now_;
      return true;
    }

    if (!memory_reserve(ProcessingKeys_::allocator_type::max_value_type_size())) {
      return false;
    }
    php_assert(current_->data->processing_keys.emplace(key, now_).second);
    return true;
  }

  void finish_key_processing(const string &key) {
    current_->data->processing_keys.erase(key);
    ic_debug("finish processing '%s'\n", key.c_str());
  }

  void detach_expiration_trace(ElementStorage_::iterator it) {
    auto range = current_->data->expiration_trace.equal_range(it->second->expiring_at);
    auto trace_it = std::find_if(range.first, range.second,
                                 [it](const ExpirationTrace_::value_type &kv) {
                                   return kv.second == it->first;
                                 });
    if (trace_it != range.second) {
      current_->data->expiration_trace.erase(trace_it);
    }
  }

  bool attach_expiration_trace(ElementStorage_::iterator it) {
    if (it->second->expiring_at == std::chrono::nanoseconds::max()) {
      return true;
    }
    if (memory_reserve(ExpirationTrace_::allocator_type::max_value_type_size())) {
      current_->data->expiration_trace.emplace_hint(current_->data->expiration_trace.end(), it->second->expiring_at, it->first);
      return true;
    }
    return false;
  }

  ElementStorage_::iterator remove_element(ElementStorage_::iterator it) {
    string removing_key = it->first;
    auto next_it = current_->data->storage.erase(it);
    DeepDestroyFromCacheVisitor{}.process(removing_key);
    return next_it;
  }

  SharedMemoryData *current_{nullptr};
  SharedMemoryDataManager data_manager_;


  struct IntrusivePtrHash {
    size_t operator()(const vk::intrusive_ptr<ElementHolder> &el) const {
      return reinterpret_cast<size_t>(el.get());
    }
  };
  std::unordered_set<vk::intrusive_ptr<ElementHolder>, IntrusivePtrHash> used_elements_;


  // request_cache_ is a special cache which allows to avoid of locking global storage
  // placed in script memory
  array<vk::not_owner_ptr<ElementHolder>> request_cache_;

  pid_t initial_process_{0};
  dl::size_type total_memory_limit_{DEFAULT_MEMORY_LIMIT};
  std::chrono::nanoseconds now_{std::chrono::nanoseconds::zero()};
};

constexpr size_t InstanceCache::DEFAULT_MEMORY_LIMIT;
constexpr double InstanceCache::MEMORY_USED_THRESHOLD;
constexpr double InstanceCache::REAL_MEMORY_USED_THRESHOLD;
constexpr double InstanceCache::FRESHNESS_ELEMENT_RATIO;
constexpr double InstanceCache::EARLY_EXPIRATION_ELEMENT_RATIO;
constexpr std::chrono::seconds InstanceCache::DEAD_PROCESSING_WORKER_TIMEOUT;
constexpr std::chrono::minutes InstanceCache::PHYSICAL_REMOVING_DELAY;

DeepMoveFromScriptToCacheVisitor::DeepMoveFromScriptToCacheVisitor() :
  Basic(*this) {
}

DeepDestroyFromCacheVisitor::DeepDestroyFromCacheVisitor() :
  Basic(*this) {
}

ShallowMoveFromCacheToScriptVisitor::ShallowMoveFromCacheToScriptVisitor() :
  Basic(*this) {
}

bool DeepMoveFromScriptToCacheVisitor::process(string &str) {
  if (str.is_const_reference_counter()) {
    return true;
  }

  if (memory_limit_exceeded_ || !memory_reserve(str.estimate_memory_usage())) {
    str = string();
    memory_limit_exceeded_ = true;
    return false;
  }

  str.make_not_shared();
  // make_not_shared may make str constant again (e.g. const empty or single char str), therefore check again
  if (str.is_const_reference_counter()) {
    php_assert(str.size() < 2);
    rollback_memory_reserve();
  } else {
    str.set_reference_counter_to_cache();
  }
  return true;
}

bool DeepDestroyFromCacheVisitor::process(string &str) {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_const_reference_counter()) {
    str.destroy_cached();
  }
  return true;
}

bool DeepMoveFromScriptToCacheVisitor::memory_reserve(dl::size_type size) {
  return InstanceCache::get().memory_reserve(size);
}

void DeepMoveFromScriptToCacheVisitor::rollback_memory_reserve() {
  InstanceCache::get().rollback_memory_reserve();
}

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int ttl) {
  return InstanceCache::get().store(key, instance_wrapper, ttl);
}

InstanceWrapperBase *instance_cache_fetch_wrapper(const string &key, bool even_if_expired) {
  return InstanceCache::get().fetch(key, even_if_expired);
}

} // namespace ic_impl_

void global_init_instance_cache_lib() {
  ic_impl_::InstanceCache::get().global_init();
}

void init_instance_cache_lib() {
  // Before each request we refresh current time point and remove expired elements
  // Time point refreshes only once for request
  ic_impl_::InstanceCache::get().refresh(logname_id);
}

void free_instance_cache_lib() {
  ic_impl_::InstanceCache::get().free(logname_id);
}

// should be called only from master
void set_instance_cache_memory_limit(dl::size_type limit) {
  ic_impl_::InstanceCache::get().set_memory_limit(limit);
}

// should be called only from master
InstanceCacheStats instance_cache_get_stats() {
  return ic_impl_::InstanceCache::get().get_stats();
}

// these function should be called from master
bool instance_cache_is_memory_swap_required() {
  return ic_impl_::InstanceCache::get().is_memory_swap_required();
}

// should be called only from master
memory_resource::MemoryStats instance_cache_get_memory_stats() {
  return ic_impl_::InstanceCache::get().get_current_memory_stats();
}

// should be called only from master
bool instance_cache_try_swap_memory(const pid_t *active_workers_begin, const pid_t *active_workers_end) {
  return ic_impl_::InstanceCache::get().try_swap_memory_resource(active_workers_begin, active_workers_end);
}

// should be called only from master
void instance_cache_purge_expired_elements() {
  ic_impl_::InstanceCache::get().purge_expired();
}

bool f$instance_cache_update_ttl(const string &key, int ttl) {
  return ic_impl_::InstanceCache::get().update_ttl(key, ttl);
}

bool f$instance_cache_delete(const string &key) {
  return ic_impl_::InstanceCache::get().del(key);
}

bool f$instance_cache_clear() {
  ic_impl_::InstanceCache::get().clear();
  return true;
}
