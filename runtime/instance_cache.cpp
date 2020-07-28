#include "runtime/instance_cache.h"

#include <chrono>
#include <forward_list>
#include <map>
#include <mutex>
#include <unordered_set>

#include "common/kprintf.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/inter-process-mutex.h"
#include "runtime/inter-process-resource.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/refcountable_php_classes.h"

namespace ic_impl_ {

//#define DEBUG_INSTANCE_CACHE

#ifdef DEBUG_INSTANCE_CACHE
  #define ic_debug kprintf
#else
inline void ic_debug(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
inline void ic_debug(const char *, ...) {}
#endif

// Дефолтное ограничение памяти для всего буффера (их 2)
static constexpr size_t DEFAULT_MEMORY_LIMIT{256u * 1024u * 1024u};
// Уровень используемой памяти, по достижении которой мы свопаем буффера
static constexpr double REAL_MEMORY_USED_THRESHOLD{0.9};
// Элементы прожившие меньше этого значения относительно ожидаемого времени жизни, не будут перезаписываться
static constexpr double FRESHNESS_ELEMENT_RATIO{0.2};
// Для элемента прожившего больше этого значения относительно ожидаемого времени жизни,
// очередной fetch вернет null (1 раз), чтобы процесс получивший его, мог предварительно обновить этот элемент
static constexpr double EARLY_EXPIRATION_ELEMENT_RATIO{0.8};
// Ограничение на время жизни элемента после его удаления сверху, чтобы он не жил слишком долго
static constexpr std::chrono::seconds DELETED_ELEMENT_LIFETIME_LIMIT{1};
// После окончания времени жизни, элемент будет удален не сразу, а через этот интервал
static constexpr std::chrono::minutes PHYSICAL_REMOVING_DELAY{1};
// Кол-во используемых бакетов для шардирования элементов
static constexpr size_t DATA_SHARDS_COUNT{997u};
// Задает шаг проверки бакетов при во время чистки кеша
static constexpr size_t SHARDS_PURGE_PERIOD{5u};

class ElementHolder;

struct CacheContext : private vk::not_copyable {
  inter_process_mutex allocator_mutex;
  memory_resource::unsynchronized_pool_resource memory_resource;
  InstanceCacheStats stats;
  std::atomic<bool> memory_swap_required{false};

  void move_to_garbage(ElementHolder *element) noexcept;
  bool has_garbage() const noexcept { return cache_garbage_ != nullptr; }
  void clear_garbage() noexcept;

  auto memory_replacement_guard(bool force_enable_disable = false) noexcept {
    dl::enter_critical_section();
    dl::set_current_script_allocator(memory_resource, force_enable_disable);
    return vk::finally([force_enable_disable] {
      dl::restore_default_script_allocator(force_enable_disable);
      dl::leave_critical_section();
    });
  }

private:
  std::atomic<ElementHolder *> cache_garbage_{nullptr};
};

class ElementHolder : private vk::thread_safe_refcnt<ElementHolder> {
public:
  using vk::thread_safe_refcnt<ElementHolder>::add_ref;
  using vk::thread_safe_refcnt<ElementHolder>::get_refcnt;

  void release() noexcept {
    if (--refcnt == 0) {
      cache_context.move_to_garbage(this);
    }
  }

  void destroy() noexcept {
    php_assert(refcnt == 0);
    cache_context.stats.elements_destroyed.fetch_add(1, std::memory_order_relaxed);
    auto &mem_resource = cache_context.memory_resource;
    this->~ElementHolder();
    mem_resource.deallocate(this, sizeof(ElementHolder));
  }

  ElementHolder(std::chrono::nanoseconds now, int64_t ttl,
                std::unique_ptr<InstanceWrapperBase> &&instance,
                CacheContext &context) noexcept:
    inserted_by_process(getpid()),
    instance_wrapper(std::move(instance)),
    cache_context(context) {
    update_time_points(now, ttl);
    cache_context.stats.elements_created.fetch_add(1, std::memory_order_relaxed);
  }

  // показывает сколько элемент прожил относительно ожидаемого времени жизни
  double freshness_ratio(std::chrono::nanoseconds now, double immortal_ratio = 0.5) const noexcept {
    // бессмертный элемент
    if (expiring_at == std::chrono::nanoseconds::max()) {
      return immortal_ratio;
    }
    if (expiring_at <= stored_at) {
      return 1.0;
    }
    const auto real_age = std::chrono::duration<double>{std::max(now, stored_at) - stored_at};
    const auto max_age = std::chrono::duration<double>{expiring_at - stored_at};
    return real_age.count() / max_age.count();
  }

  void update_time_points(std::chrono::nanoseconds now, int64_t ttl) noexcept {
    stored_at = std::max(now, stored_at);
    expiring_at = ttl > 0 ? stored_at + std::chrono::seconds{ttl} : std::chrono::nanoseconds::max();
    early_fetch_performed = false;
  }

  std::chrono::nanoseconds stored_at{std::chrono::nanoseconds::min()};
  std::chrono::nanoseconds expiring_at{std::chrono::nanoseconds::max()};
  bool early_fetch_performed{false};
  const pid_t inserted_by_process{0};

  std::unique_ptr<InstanceWrapperBase> instance_wrapper;
  CacheContext &cache_context;

  // Список удаленных элементов
  std::atomic<ElementHolder *> next_in_garbage_list{nullptr};
};

using ElementStorage_ = memory_resource::stl::map<string, vk::intrusive_ptr<ElementHolder>, memory_resource::unsynchronized_pool_resource, stl_string_less>;

struct SharedDataStorages : private vk::not_copyable {
  explicit SharedDataStorages(memory_resource::unsynchronized_pool_resource &resource) :
    storage(ElementStorage_::allocator_type{resource}) {
  }

  inter_process_mutex storage_mutex;
  ElementStorage_ storage;
  std::atomic<bool> is_storage_empty{true};
};

void CacheContext::move_to_garbage(ElementHolder *element) noexcept {
  php_assert(element->next_in_garbage_list == nullptr);
  // Собираем мусор в cache_context.cache_garbage, очистка будет позже под блокировкой
  auto *next = cache_garbage_.load();
  do {
    element->next_in_garbage_list.store(next);
  } while (!cache_garbage_.compare_exchange_strong(next, element));
}

void CacheContext::clear_garbage() noexcept {
  auto element = cache_garbage_.exchange(nullptr);

  while (element) {
    auto next = element->next_in_garbage_list.load();
    element->destroy();
    element = next;
  }
}

class SharedMemoryData : vk::not_copyable {
public:
  void init(size_t pool_size) noexcept {
    php_assert(!data_shards_);
    php_assert(!cache_context_);
    php_assert(!shared_memory_);
    shared_memory_pool_size_ = pool_size;
    share_memory_full_size_ = get_context_size() + get_data_size() + shared_memory_pool_size_;
    shared_memory_ = mmap(nullptr, share_memory_full_size_, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    php_assert(shared_memory_);
    construct_data_inplace();
  }

  void reset() noexcept {
    destroy_data();
    construct_data_inplace();
  }

  void destroy() noexcept {
    destroy_data();
    data_shards_ = nullptr;
    cache_context_ = nullptr;
  }

  SharedDataStorages &get_data(const string &key) noexcept {
    php_assert(data_shards_);
    return data_shards_[static_cast<uint32_t>(key.hash()) % DATA_SHARDS_COUNT];
  }

  SharedDataStorages *get_data_shards() noexcept {
    return data_shards_;
  }

  static constexpr size_t get_data_shards_count() noexcept {
    return DATA_SHARDS_COUNT;
  }

  CacheContext &get_context() noexcept {
    php_assert(cache_context_);
    return *cache_context_;
  }

private:
  void destroy_data() noexcept {
    php_assert(data_shards_);
    for (size_t i = 0; i != DATA_SHARDS_COUNT; ++i) {
      // to avoid of calling all destructors, just free storage_mutex
      data_shards_[i].storage_mutex.~inter_process_mutex();
    }

    php_assert(cache_context_);
    cache_context_->~CacheContext();
  }

  void construct_data_inplace() noexcept {
    cache_context_ = new(shared_memory_) CacheContext();
    uint8_t *data_storage_mem = static_cast<uint8_t *>(shared_memory_) + get_context_size();
    cache_context_->memory_resource.init(data_storage_mem + get_data_size(), shared_memory_pool_size_);
    data_shards_ = reinterpret_cast<SharedDataStorages *>(data_storage_mem);
    for (size_t i = 0; i != DATA_SHARDS_COUNT; ++i) {
      new(&data_shards_[i]) SharedDataStorages{cache_context_->memory_resource};
    }
  }

  static constexpr size_t get_context_size() noexcept {
    return (sizeof(CacheContext) + 7) & -8;
  }

  static constexpr size_t get_data_size() noexcept {
    return (sizeof(SharedDataStorages) * DATA_SHARDS_COUNT + 7) & -8;
  }

  void *shared_memory_{nullptr};
  size_t share_memory_full_size_{0};
  size_t shared_memory_pool_size_{0};
  CacheContext *cache_context_{nullptr};
  SharedDataStorages *data_shards_{nullptr};
};

struct {
  size_t total_memory_limit{DEFAULT_MEMORY_LIMIT};
} static instance_cache_settings;

class InstanceCache {
private:
  InstanceCache() :
    now_(std::chrono::system_clock::now().time_since_epoch()) {
  }

public:
  static InstanceCache &get() {
    static InstanceCache cache;
    return cache;
  }

  void global_init() {
    php_assert(!current_ && !context_);
    data_manager_.init(instance_cache_settings.total_memory_limit);
  }

  void refresh() {
    php_assert(!current_ && !context_);
    update_now();
    current_ = data_manager_.acquire_current_resource();
    context_ = &current_->get_context();
  }

  void update_now() {
    now_ = std::chrono::system_clock::now().time_since_epoch();
  }

  void free() {
    php_assert(current_ && context_);
    sync_delayed();

    // request_cache_ и storing_delayed_ используют память скрипта
    storing_delayed_.clear();
    request_cache_.clear();
    // used_elements использует heap память
    used_elements_.clear();

    if (context_->has_garbage()) {
      std::unique_lock<inter_process_mutex> allocator_lock{context_->allocator_mutex, std::try_to_lock};
      if (allocator_lock) {
        auto shared_memory_guard = context_->memory_replacement_guard();
        context_->clear_garbage();
      }
    }
    data_manager_.release_resource(current_);
    current_ = nullptr;
    context_ = nullptr;
  }

  bool store(const string &key, const InstanceWrapperBase &instance_wrapper, int64_t ttl) noexcept {
    ic_debug("store '%s'\n", key.c_str());
    php_assert(current_ && context_);
    if (context_->memory_swap_required) {
      return false;
    }

    sync_delayed();
    // различные сервисные вещи, которые можно делать вне блокировок
    auto &data = current_->get_data(key);
    update_now();
    if (is_element_insertion_can_be_skipped(data, key)) {
      return false;
    }

    DeepMoveFromScriptToCacheVisitor detach_processor{context_->memory_resource};
    const ElementHolder *inserted_element = try_insert_element_into_cache(
      data, key, ttl, instance_wrapper, detach_processor);

    if (!inserted_element) {
      // Не удалось вставить элемент из-за каких-то проблем: память, лимит глубины
      if (unlikely(!detach_processor.is_ok())) {
        fire_warning(detach_processor, instance_wrapper.get_class());
        return false;
      }
      // Если не смогли захватить блокировку, сохраняем инстанс в контейнер в памяти скрипта, попроуем позже
      class_instance<DelayedInstance> delayed_instance;
      delayed_instance.alloc().get()->ttl = ttl;
      delayed_instance.get()->instance_wrapper = instance_wrapper.clone_on_script_memory();
      storing_delayed_.set_value(key, std::move(delayed_instance));
      context_->stats.elements_storing_delayed_due_mutex.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
    ic_debug("element '%s' was successfully inserted\n", key.c_str());
    context_->stats.elements_stored.fetch_add(1, std::memory_order_relaxed);
    // request_cache_ использует память скрипта
    request_cache_.set_value(key, inserted_element);
    return true;
  }

  const InstanceWrapperBase *fetch(const string &key, bool even_if_expired) {
    php_assert(current_ && context_);
    sync_delayed();
    // storing_delayed_ использует память скрипта
    if (const auto *delayed_instance = storing_delayed_.find_value(key)) {
      ic_debug("fetch '%s' from delayed cache\n", key.c_str());
      return delayed_instance->get()->instance_wrapper.get();
    }
    // request_cache_ использует память скрипта
    if (const ElementHolder *const *cached_element_ptr = request_cache_.find_value(key)) {
      ic_debug("fetch '%s' from request cache\n", key.c_str());
      return (*cached_element_ptr)->instance_wrapper.get();
    }

    vk::intrusive_ptr<ElementHolder> element;
    bool element_logically_expired = false;
    {
      auto &data = current_->get_data(key);
      std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
      auto it = data.storage.find(key);
      if (it == data.storage.end()) {
        ic_debug("can't fetch '%s' because it is absent\n", key.c_str());
        context_->stats.elements_missed.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
      }

      update_now();
      // Если прошло больше чем EARLY_EXPIRATION_ELEMENT_RATIO от общего времени жизни элемента
      // возвращяем null первому попавшемуся процессу, чтобы он обновил его заранее
      if (!it->second->early_fetch_performed &&
          it->second->freshness_ratio(now_) >= EARLY_EXPIRATION_ELEMENT_RATIO) {
        it->second->early_fetch_performed = true;
        context_->stats.elements_missed_earlier.fetch_add(1, std::memory_order_relaxed);
        ic_debug("can't fetch '%s' because less than %f of total time is left\n",
                 key.c_str(), EARLY_EXPIRATION_ELEMENT_RATIO);
        return nullptr;
      }
      element_logically_expired = it->second->expiring_at <= now_;
      if (element_logically_expired) {
        if (even_if_expired) {
          context_->stats.elements_logically_expired_but_fetched.fetch_add(1, std::memory_order_relaxed);
          ic_debug("fetch logically expired element '%s'\n", key.c_str());
        } else {
          context_->stats.elements_logically_expired_and_ignored.fetch_add(1, std::memory_order_relaxed);
          ic_debug("can't fetch '%s' because element was logically expired\n", key.c_str());
          return nullptr;
        }
      } else {
        context_->stats.elements_fetched.fetch_add(1, std::memory_order_relaxed);
        ic_debug("fetch '%s' from inter process cache\n", key.c_str());
      }

      element = it->second;
    }

    // не кешируем логически истекшие элементы
    if (!element_logically_expired) {
      // request_cache_ использует память скрипта
      request_cache_.set_value(key, element.get());
    }

    const auto *result = element->instance_wrapper.get();
    // used_elements_ использует heap memory, будет держать элемент до конца запроса
    used_elements_.emplace(std::move(element));
    return result;
  }

  bool update_ttl(const string &key, int64_t ttl) {
    php_assert(current_ && context_);
    ic_debug("update_ttl '%s', new ttl '%ld'\n", key.c_str(), ttl);
    sync_delayed();
    // storing_delayed_ использует память скрипта
    // Не очень понятно, что делать с storing_delayed_, пока пусть будет так,
    // но возможно стоит его или чистатить от этого ключа или вообще ничего не делать
    if (const auto *delayed_instance = storing_delayed_.find_value(key)) {
      delayed_instance->get()->ttl = ttl;
    }

    auto &data = current_->get_data(key);
    update_now();
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    if (it == data.storage.end()) {
      return false;
    }

    it->second->update_time_points(now_, ttl);
    return true;
  }

  bool del(const string &key) {
    php_assert(current_ && context_);
    ic_debug("delete '%s'\n", key.c_str());
    sync_delayed();
    // request_cache_ и storing_delayed_ используют память скрипта
    storing_delayed_.unset(key);
    request_cache_.unset(key);
    auto &data = current_->get_data(key);
    update_now();
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    if (it == data.storage.end()) {
      return false;
    }

    // расчитываем expiring_at так, что бы следующий fetch вернул false
    constexpr double SCALE = 1.0 / EARLY_EXPIRATION_ELEMENT_RATIO;
    auto new_element_ttl = std::chrono::duration_cast<std::chrono::nanoseconds>((now_ - it->second->stored_at) * SCALE);
    auto new_expiring_at = std::chrono::duration_cast<std::chrono::nanoseconds>(it->second->stored_at + new_element_ttl);
    new_expiring_at = std::min(new_expiring_at, now_ + DELETED_ELEMENT_LIFETIME_LIMIT);
    it->second->expiring_at = std::max(new_expiring_at, it->second->stored_at);
    return true;
  }

  void force_release_all_resources() {
    data_manager_.force_release_all_resources();
  }

  // this function should be called only from master
  void purge_expired() {
    update_now();
    const auto now_with_delay = now_ - PHYSICAL_REMOVING_DELAY;

    auto &current_data = data_manager_.get_current_resource();
    auto &context = current_data.get_context();
    // заменяем дефолтный script allocator
    // так как этот вызов происходит из master процесса
    // нам необходимо явно активировать и деактивировать его
    auto shared_memory_guard = context.memory_replacement_guard(true);

    auto *data_shards = current_data.get_data_shards();
    const size_t shards_count = current_data.get_data_shards_count();
    for (size_t shard_id = purge_shard_offset_; shard_id < shards_count; shard_id += SHARDS_PURGE_PERIOD) {
      auto &data_shard = data_shards[shard_id];
      if (data_shard.is_storage_empty.load(std::memory_order_relaxed)) {
        continue;
      }
      {
        std::lock_guard<inter_process_mutex> shared_data_lock{data_shard.storage_mutex};
        if (std::none_of(data_shard.storage.begin(), data_shard.storage.end(),
                         [now_with_delay](const auto &stored_element) {
                           return stored_element.second->expiring_at <= now_with_delay;
                         })) {
          continue;
        }
      }

      // Блокировать именно в таком порядке и не переносить allocator_lock в if ниже, иначе будет дедлок!
      std::lock_guard<inter_process_mutex> allocator_lock{context.allocator_mutex};
      std::lock_guard<inter_process_mutex> shared_data_lock{data_shard.storage_mutex};
      for (auto it = data_shard.storage.begin(); it != data_shard.storage.end();) {
        if (it->second->expiring_at <= now_with_delay) {
          ic_debug("purge '%s'\n", it->first.c_str());
          string removing_key = it->first;
          it = data_shard.storage.erase(it);
          DeepDestroyFromCacheVisitor{}.process(removing_key);
          context.stats.elements_expired.fetch_add(1, std::memory_order_relaxed);
          context.stats.elements_cached.fetch_sub(1, std::memory_order_relaxed);
        } else {
          ++it;
        }
      }
      data_shard.is_storage_empty.store(data_shard.storage.empty(), std::memory_order_relaxed);
    }

    purge_shard_offset_ = (purge_shard_offset_ + 1) % SHARDS_PURGE_PERIOD;

    std::lock_guard<inter_process_mutex> allocator_lock{context.allocator_mutex};
    context.clear_garbage();
    last_memory_stats_ = context.memory_resource.get_memory_stats();
  }

  // this function should be called only from master
  InstanceCacheSwapStatus try_swap_memory_resource() {
    const auto &memory_stats = get_last_memory_stats();
    const auto threshold = REAL_MEMORY_USED_THRESHOLD * static_cast<double>(memory_stats.memory_limit);
    if (static_cast<double>(memory_stats.real_memory_used) < threshold &&
        !data_manager_.get_current_resource().get_context().memory_swap_required) {
      return InstanceCacheSwapStatus::no_need;
    }
    return data_manager_.try_switch_to_next_unused_resource()
           ? InstanceCacheSwapStatus::swap_is_finished
           : InstanceCacheSwapStatus::swap_is_forbidden;
  }

  // this function should be called only from master
  const InstanceCacheStats &get_stats() {
    return data_manager_.get_current_resource().get_context().stats;
  }

  // this function should be called only from master
  const memory_resource::MemoryStats &get_last_memory_stats() const noexcept {
    return last_memory_stats_;
  }

private:
  bool is_element_insertion_can_be_skipped(SharedDataStorages &data, const string &key) const {
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    // разрешаем пропустить вставку элемента, если он недавно был вставлен другим процессом
    if (it != data.storage.end() &&
        it->second->freshness_ratio(now_) < FRESHNESS_ELEMENT_RATIO &&
        it->second->inserted_by_process != getpid()) {
      ic_debug("skip '%s' because it was recently updated\n", key.c_str());
      context_->stats.elements_storing_skipped_due_recent_update.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

  void sync_delayed() noexcept {
    php_assert(current_ && context_);
    if (context_->memory_swap_required) {
      return;
    }

    DeepMoveFromScriptToCacheVisitor detach_processor{context_->memory_resource};
    for (auto it = storing_delayed_.cbegin(); it != storing_delayed_.cend(); it = storing_delayed_.cbegin()) {
      php_assert(it.is_string_key());
      const auto &key = it.get_string_key();
      const auto &delayed_instance = *it.get_value().get();
      auto &data = current_->get_data(key);
      update_now();
      if (is_element_insertion_can_be_skipped(data, key)) {
        storing_delayed_.unset(key);
        continue;
      }
      const ElementHolder *inserted_element = try_insert_element_into_cache(
        data, key, delayed_instance.ttl,
        *delayed_instance.instance_wrapper, detach_processor);
      if (!inserted_element) {
        if (likely(detach_processor.is_ok())) {
          // Не удалось захватить блокировку аллокатора, попробуем позже
          return;
        }
        fire_warning(detach_processor, delayed_instance.instance_wrapper->get_class());
        if (detach_processor.is_memory_limit_exceeded()) {
          return;
        }
      } else {
        ic_debug("element '%s' was successfully inserted with delay\n", key.c_str());
        context_->stats.elements_stored_with_delay.fetch_add(1, std::memory_order_relaxed);
        // request_cache_ использует память скрипта
        request_cache_.set_value(key, inserted_element);
      }
      storing_delayed_.unset(key);
    }
  }

  ElementHolder *try_insert_element_into_cache(SharedDataStorages &data,
                                               const string &key_in_script_memory, int64_t ttl,
                                               const InstanceWrapperBase &instance_wrapper,
                                               DeepMoveFromScriptToCacheVisitor &detach_processor) noexcept {
    // подменяем аллокатор
    auto shared_memory_guard = context_->memory_replacement_guard();

    std::unique_lock<inter_process_mutex> allocator_lock{context_->allocator_mutex, std::try_to_lock};
    // Лочим строго перед storage_mutex, что бы не было дедлока
    if (!allocator_lock) {
      return nullptr;
    }

    // захватили блокировку аллокатора, поэтому после можем спокойно почистить мусор
    auto clear_garbage = vk::finally([this] { context_->clear_garbage(); });

    // переносим инстанс в шареную память
    if (auto cached_instance_wrapper = instance_wrapper.clone_and_detach_shared_ref(detach_processor)) {
      if (void *mem = detach_processor.prepare_raw_memory(sizeof(ElementHolder))) {
        vk::intrusive_ptr<ElementHolder> element{new(mem) ElementHolder{now_, ttl, std::move(cached_instance_wrapper), *context_}};
        std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
        auto it = data.storage.find(key_in_script_memory);
        if (it == data.storage.end()) {
          string key_in_shared_memory = key_in_script_memory;
          if (unlikely(!detach_processor.process(key_in_shared_memory))) {
            return nullptr;
          }
          constexpr auto node_max_size = ElementStorage_::allocator_type::max_value_type_size();
          if (unlikely(!detach_processor.is_enough_memory_for(node_max_size))) {
            DeepDestroyFromCacheVisitor{}.process(key_in_shared_memory);
            return nullptr;
          }

          it = data.storage.emplace(std::move(key_in_shared_memory), vk::intrusive_ptr<ElementHolder>{}).first;
          data.is_storage_empty.store(false, std::memory_order_relaxed);
          context_->stats.elements_cached.fetch_add(1, std::memory_order_relaxed);
        }
        // заменяем элемент и сохраняем предыдущий элемент в used_elements_,
        // это позволит освободить его вне storage_mutex лока
        it->second.swap(element);
        if (element) {
          // used_elements_ использует heap для внутренних аллокаций
          used_elements_.emplace(std::move(element));
        }
        used_elements_.emplace(it->second);
        return it->second.get();
      }
    }
    return nullptr;
  }

  void fire_warning(const DeepMoveFromScriptToCacheVisitor &detach_processor, const char *class_name) noexcept {
    if (detach_processor.is_depth_limit_exceeded()) {
      php_warning("Depth limit exceeded on cloning instance of class '%s' into cache", class_name);
    }
    if (detach_processor.is_memory_limit_exceeded()) {
      php_warning("Memory limit exceeded on saving instance of class '%s' into cache", class_name);
      context_->memory_swap_required = true;
    }
  }

  SharedMemoryData *current_{nullptr};
  CacheContext *context_{nullptr};
  InterProcessResourceManager<SharedMemoryData, 2> data_manager_;


  struct IntrusivePtrHash {
    size_t operator()(const vk::intrusive_ptr<ElementHolder> &el) const {
      return reinterpret_cast<size_t>(el.get());
    }
  };

  // Хранилище когда-либо используемых элементов в скрипте (в рамках запроса)
  // Сохраняем сюда элементы, чтобы быть уверенными, что они не пропадут в самый неожиданный момент
  // std::unordered_set использует heap память
  // vk::intrusive_ptr<ElementHolder> использует shared память
  std::unordered_set<vk::intrusive_ptr<ElementHolder>, IntrusivePtrHash> used_elements_;

  // Локальный кеш, позволят получать элементы без лишних блокировок storage_mutex
  // Использует память скрипта
  array<const ElementHolder *> request_cache_;

  // Контейнер для инстансов, которые не удалось сохранить с первого раза из-за блокировки аллокатора
  // Использует память скрипта. Завернуто в class_instance так как array не умеет работать с unique_ptr
  struct DelayedInstance : refcountable_php_classes<DelayedInstance> {
    std::unique_ptr<InstanceWrapperBase> instance_wrapper;
    int64_t ttl{0};
  };
  array<class_instance<DelayedInstance>> storing_delayed_;

  std::chrono::nanoseconds now_{std::chrono::nanoseconds::zero()};
  memory_resource::MemoryStats last_memory_stats_;
  size_t purge_shard_offset_{0};
};

DeepMoveFromScriptToCacheVisitor::DeepMoveFromScriptToCacheVisitor(memory_resource::unsynchronized_pool_resource &memory_pool) noexcept:
  Basic(*this),
  memory_pool_(memory_pool) {
}

DeepDestroyFromCacheVisitor::DeepDestroyFromCacheVisitor() :
  Basic(*this) {
}

bool DeepMoveFromScriptToCacheVisitor::process(string &str) {
  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return true;
  }

  if (unlikely(!is_enough_memory_for(str.estimate_memory_usage()))) {
    str = string();
    memory_limit_exceeded_ = true;
    return false;
  }

  str.make_not_shared();
  // make_not_shared may make str constant again (e.g. const empty or single char str), therefore check again
  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    php_assert(str.size() < 2);
  } else {
    php_assert(str.get_reference_counter() == 1);
    str.set_reference_counter_to(ExtraRefCnt::for_instance_cache);
  }
  return true;
}

bool DeepDestroyFromCacheVisitor::process(string &str) {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    str.force_destroy(ExtraRefCnt::for_instance_cache);
  }
  return true;
}

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int64_t ttl) {
  return InstanceCache::get().store(key, instance_wrapper, ttl);
}

const InstanceWrapperBase *instance_cache_fetch_wrapper(const string &key, bool even_if_expired) {
  return InstanceCache::get().fetch(key, even_if_expired);
}

} // namespace ic_impl_

void global_init_instance_cache_lib() {
  ic_impl_::InstanceCache::get().global_init();
}

void init_instance_cache_lib() {
  ic_impl_::InstanceCache::get().refresh();
}

void free_instance_cache_lib() {
  ic_impl_::InstanceCache::get().free();
}

// should be called only from master
void set_instance_cache_memory_limit(size_t limit) {
  ic_impl_::instance_cache_settings.total_memory_limit = limit;
}

// should be called only from master
InstanceCacheSwapStatus instance_cache_try_swap_memory() {
  return ic_impl_::InstanceCache::get().try_swap_memory_resource();
}

// should be called only from master
const InstanceCacheStats &instance_cache_get_stats() {
  return ic_impl_::InstanceCache::get().get_stats();
}

// should be called only from master
const memory_resource::MemoryStats &instance_cache_get_memory_stats() {
  return ic_impl_::InstanceCache::get().get_last_memory_stats();
}

// should be called only from master
void instance_cache_purge_expired_elements() {
  ic_impl_::InstanceCache::get().purge_expired();
}

void instance_cache_release_all_resources_acquired_by_this_proc() {
  ic_impl_::InstanceCache::get().force_release_all_resources();
}

bool f$instance_cache_update_ttl(const string &key, int64_t ttl) {
  return ic_impl_::InstanceCache::get().update_ttl(key, ttl);
}

bool f$instance_cache_delete(const string &key) {
  return ic_impl_::InstanceCache::get().del(key);
}
