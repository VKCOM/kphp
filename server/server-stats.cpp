// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <atomic>
#include <iomanip>
#include <new>

#include "common/functional/identity.h"
#include "common/smart_iterators/transform_iterator.h"
#include "common/wrappers/memory-utils.h"
#include "net/net-events.h"

#include "runtime/curl.h"

#include "server/workers-control.h"

#include "server/server-stats.h"
#include "server/statshouse/statshouse-client.h"
#include "server/statshouse/worker-stats-buffer.h"

namespace {

template<class T>
struct WithStatType {
  using StatType = T;
};

struct ScriptSamples : WithStatType<uint64_t> {
  enum class Key {
    memory_used = 0,
    real_memory_used,
    total_allocated_by_curl,
    outgoing_queries,
    outgoing_long_queries,
    working_time,
    net_time,
    script_time,
    types_count
  };
};

struct QueriesStat : WithStatType<uint64_t> {
  enum class Key {
    incoming_queries,
    outgoing_queries,
    outgoing_long_queries,
    script_time,
    net_time,
    types_count
  };
};

struct JobSamples : WithStatType<uint64_t> {
  enum class Key {
    wait_time = 0,
    request_memory_usage,
    request_real_memory_usage,
    response_memory_usage,
    response_real_memory_usage,
    types_count
  };
};

struct JobCommonMemorySamples : WithStatType<uint64_t> {
  enum class Key {
    common_request_memory_usage = 0,
    common_request_real_memory_usage,
    types_count
  };
};

struct MallocStat : WithStatType<uint64_t> {
  enum class Key {
    non_mmaped_allocated_bytes = 0,
    non_mmaped_free_bytes,
    mmaped_bytes,
    types_count
  };
};

struct HeapStat : WithStatType<uint64_t> {
  enum class Key {
    script_heap_memory_usage = 0,
    curl_memory_currently_usage,
    types_count
  };
};

struct VMStat : WithStatType<uint32_t> {
  enum class Key {
    vm_peak_kb,
    vm_kb,
    rss_peak_kb,
    rss_kb,
    shm_kb,
    types_count
  };
};

struct IdleStat : WithStatType<double> {
  enum class Key {
    tot_idle_time,
    tot_idle_percent,
    recent_idle_percent,
    types_count
  };
};

struct MiscStat : WithStatType<uint64_t> {
  enum {
    worker_idle = 0,
    worker_running = 1,
    worker_waiting_net = 2
  };
  enum class Key {
    process_pid,
    max_special_connections,
    active_special_connections,
    worker_status,
    worker_activity_counter,
    types_count
  };
};

template<class E, class T = typename E::StatType>
struct EnumTable : std::array<T, static_cast<size_t>(E::Key::types_count)> {
  using Base = std::array<T, static_cast<size_t>(E::Key::types_count)>;

  using Base::operator[];

  const T &operator[](typename E::Key key) const noexcept {
    return (*this)[static_cast<size_t>(key)];
  }

  T &operator[](typename E::Key key) noexcept {
    return (*this)[static_cast<size_t>(key)];
  }
};

template<class T>
struct Percentiles {
  T p50{};
  T p95{};
  T p99{};
  T max{};
  T sum{};

  template<class I, class Mapper = vk::identity>
  void update_percentiles(I first, I last, const Mapper &mapper = {}) noexcept {
    const auto size = last - first;
    set_percentile<50>(p50, first, size, mapper);
    set_percentile<95>(p95, first, size, mapper);
    set_percentile<99>(p99, first, size, mapper);
    set_percentile<100>(max, first, size, mapper);
    sum = std::accumulate(vk::make_transform_iterator(mapper, first), vk::make_transform_iterator(mapper, last), T{});
  }

private:
  template<size_t P, class I, class Mapper>
  static void set_percentile(T &out, I first, std::ptrdiff_t size, const Mapper &mapper) noexcept {
    if (size) {
      const auto index = P * (size - 1) / 100;
      std::nth_element(first, first + index, first + size);
      out = mapper(first[index]);
    } else {
      out = T{};
    }
  }
};

EnumTable<VMStat> get_virtual_memory_stat() noexcept {
  EnumTable<VMStat> result;
  const auto mem_stats = get_self_mem_stats();
  result[VMStat::Key::vm_peak_kb] = mem_stats.vm_peak;
  result[VMStat::Key::vm_kb] = mem_stats.vm;
  result[VMStat::Key::rss_peak_kb] = mem_stats.rss_peak;
  result[VMStat::Key::rss_kb] = mem_stats.rss;
  result[VMStat::Key::shm_kb] = mem_stats.rss_shmem + mem_stats.rss_file;
  return result;
}

EnumTable<MallocStat> get_malloc_stat() noexcept {
  EnumTable<MallocStat> result;
  const auto malloc_info = get_malloc_stats();
  result[MallocStat::Key::non_mmaped_allocated_bytes] = malloc_info.uordblks;
  result[MallocStat::Key::non_mmaped_free_bytes] = malloc_info.fordblks;
  result[MallocStat::Key::mmaped_bytes] = malloc_info.hblkhd;
  return result;
}

EnumTable<HeapStat> get_heap_stat() noexcept {
  EnumTable<HeapStat> result;
  result[HeapStat::Key::curl_memory_currently_usage] = vk::singleton<CurlMemoryUsage>::get().currently_allocated;
  result[HeapStat::Key::script_heap_memory_usage] = dl::get_heap_memory_used();
  return result;
}

EnumTable<IdleStat> get_idle_stat() noexcept {
  EnumTable<IdleStat> result;
  result[IdleStat::Key::tot_idle_time] = epoll_total_idle_time();
  const int uptime = get_uptime();
  result[IdleStat::Key::tot_idle_percent] = uptime > 0 ? result[IdleStat::Key::tot_idle_time] / uptime * 100 : 0;
  const double average_idle_quotient = epoll_average_idle_quotient();
  result[IdleStat::Key::recent_idle_percent] = average_idle_quotient > 0 ? epoll_average_idle_time() / average_idle_quotient * 100 : 0;
  return result;
}

EnumTable<QueriesStat> make_queries_stat(uint64_t script_queries, uint64_t long_script_queries, uint64_t script_time_ns, uint64_t net_time_ns) noexcept {
  EnumTable<QueriesStat> result;
  result[QueriesStat::Key::incoming_queries] = 1;
  result[QueriesStat::Key::outgoing_queries] = script_queries;
  result[QueriesStat::Key::outgoing_long_queries] = long_script_queries;
  result[QueriesStat::Key::script_time] = script_time_ns;
  result[QueriesStat::Key::net_time] = net_time_ns;
  return result;
}

template<class T>
struct AggregatedSamples;

template<class T>
struct SharedSamples : private vk::not_copyable {
public:
  void init(std::mt19937 *gen) noexcept {
    gen_ = gen;
  }

  void add_sample(T value, size_t sample_index) noexcept {
    if (sample_index >= samples_.size()) {
      // the zero value doesn't override anything
      if (value == T{}) {
        return;
      }
      sample_index = std::uniform_int_distribution<size_t>{0, sample_index}(*gen_);
    }
    if (sample_index < samples_.size()) {
      samples_[sample_index].store(value, std::memory_order_relaxed);
    }
  }

private:
  friend struct AggregatedSamples<T>;

  std::mt19937 *gen_{nullptr};
  std::array<std::atomic<T>, 1024> samples_{};
};

template<class E>
struct SharedSamplesBundle : EnumTable<E, SharedSamples<typename E::StatType>>, private vk::not_copyable {
public:
  explicit SharedSamplesBundle(std::mt19937 *gen) noexcept {
    for (auto &s : *this) {
      s.init(gen);
    }
  }

  void add_sample(const EnumTable<E> &sample) noexcept {
    const size_t sample_index = last_sample.fetch_add(1, std::memory_order_acq_rel);
    for (size_t i = 0; i != this->size(); ++i) {
      (*this)[i].add_sample(sample[i], sample_index);
    }
  }

  std::atomic<uint64_t> last_sample{0};
};

struct WorkerSharedStats : private vk::not_copyable {
  explicit WorkerSharedStats(std::mt19937 *gen) noexcept:
    script_samples(gen) {
  }

  void add_request_stats(const EnumTable<QueriesStat> &queries, script_error_t error,
                         uint64_t memory_used, uint64_t real_memory_used, uint64_t curl_total_allocated) noexcept {
    errors[static_cast<size_t>(error)].fetch_add(1, std::memory_order_relaxed);

    for (size_t i = 0; i != queries.size(); ++i) {
      total_queries_stat[i].fetch_add(queries[i], std::memory_order_relaxed);
    }

    EnumTable<ScriptSamples> sample;
    sample[ScriptSamples::Key::memory_used] = memory_used;
    sample[ScriptSamples::Key::real_memory_used] = real_memory_used;
    sample[ScriptSamples::Key::total_allocated_by_curl] = curl_total_allocated;
    sample[ScriptSamples::Key::outgoing_queries] = queries[QueriesStat::Key::outgoing_queries];
    sample[ScriptSamples::Key::outgoing_long_queries] = queries[QueriesStat::Key::outgoing_long_queries];
    sample[ScriptSamples::Key::working_time] = queries[QueriesStat::Key::net_time] + queries[QueriesStat::Key::script_time];
    sample[ScriptSamples::Key::net_time] = queries[QueriesStat::Key::net_time];
    sample[ScriptSamples::Key::script_time] = queries[QueriesStat::Key::script_time];
    script_samples.add_sample(sample);
  }

  std::array<std::atomic<uint32_t>, static_cast<size_t>(script_error_t::errors_count)> errors{};

  EnumTable<QueriesStat, std::atomic<QueriesStat::StatType>> total_queries_stat;
  SharedSamplesBundle<ScriptSamples> script_samples;
};

struct JobWorkerSharedStats : WorkerSharedStats {
  explicit JobWorkerSharedStats(std::mt19937 *gen) noexcept:
    WorkerSharedStats(gen),
    job_samples(gen),
    job_common_memory_samples(gen) {
  }

  void add_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used, uint64_t response_real_memory_used) noexcept {
    EnumTable<JobSamples> sample;
    sample[JobSamples::Key::wait_time] = job_wait_ns;
    sample[JobSamples::Key::request_memory_usage] = request_memory_used;
    sample[JobSamples::Key::request_real_memory_usage] = request_real_memory_used;
    sample[JobSamples::Key::response_memory_usage] = response_memory_used;
    sample[JobSamples::Key::response_real_memory_usage] = response_real_memory_used;
    job_samples.add_sample(sample);
  }

  void add_job_common_memory_stats(uint64_t common_request_memory_used, uint64_t common_request_real_memory_used) noexcept {
    EnumTable<JobCommonMemorySamples> sample;
    sample[JobCommonMemorySamples::Key::common_request_memory_usage] = common_request_memory_used;
    sample[JobCommonMemorySamples::Key::common_request_real_memory_usage] = common_request_real_memory_used;
    job_common_memory_samples.add_sample(sample);
  }

  SharedSamplesBundle<JobSamples> job_samples;
  SharedSamplesBundle<JobCommonMemorySamples> job_common_memory_samples;
};

template<class T>
struct AggregatedSamples : vk::not_copyable {
public:
  using Sample = std::pair<T, std::chrono::steady_clock::time_point>;

  AggregatedSamples() noexcept:
    last_(samples_.begin()) {
  }

  void init(std::mt19937 *gen) noexcept {
    gen_ = gen;
  }

  void recalc(SharedSamples<T> &samples, size_t samples_count,
              std::chrono::steady_clock::time_point now_tp) noexcept {
    last_ = std::remove_if(samples_.begin(), last_,
                           [now_tp](const Sample &s) { return now_tp - s.second > std::chrono::minutes{1}; });
    samples_count = std::min(samples_count, samples.samples_.size());

    while (last_ != samples_.end() && samples_count) {
      const Sample sample{samples.samples_[--samples_count].exchange(T{}, std::memory_order_relaxed), now_tp};
      if (sample.first != T{}) {
        *last_++ = sample;
      }
    }

    std::uniform_int_distribution<size_t> distrib(0, samples_.size());
    while (samples_count) {
      const Sample sample{samples.samples_[--samples_count].exchange(T{}, std::memory_order_relaxed), now_tp};
      if (sample.first != T{}) {
        const size_t index = distrib(*gen_);
        if (index < samples_.size()) {
          samples_[index] = sample;
        }
        // the zero value doesn't affect the sample
        distrib.param(std::uniform_int_distribution<size_t>::param_type{0, distrib.max() + 1});
      }
    }
    percentiles.update_percentiles(samples_.begin(), last_, [](const Sample &s) { return s.first; });
  }

  Percentiles<T> percentiles;

private:
  std::array<Sample, 4096> samples_{};
  typename std::array<Sample, 4096>::iterator last_{};
  std::mt19937 *gen_{nullptr};
};

template<class E>
struct AggregatedSamplesBundle : EnumTable<E, AggregatedSamples<typename E::StatType>>, private vk::not_copyable {
public:
  explicit AggregatedSamplesBundle(std::mt19937 *gen) noexcept {
    for (auto &s : *this) {
      s.init(gen);
    }
  }

  void recalc(SharedSamplesBundle<E> &samples, std::chrono::steady_clock::time_point now_tp) noexcept {
    const size_t last_sample = samples.last_sample.load(std::memory_order_acquire);
    for (size_t i = 0; i != this->size(); ++i) {
      (*this)[i].recalc(samples[i], last_sample, now_tp);
    }
    samples.last_sample.store(0, std::memory_order_release);
  }
};

template<class E>
struct WorkerStatsBundle : EnumTable<E, std::array<std::atomic<typename E::StatType>, WorkersControl::max_workers_count>> {
  void set_worker_stats(const EnumTable<E> &worker_stat, uint16_t worker_index) noexcept {
    for (size_t i = 0; i != this->size(); ++i) {
      set_stat(static_cast<typename E::Key>(i), worker_index, worker_stat[i]);
    }
  }

  void add_worker_stats(const EnumTable<E> &worker_stat, uint16_t worker_index) noexcept {
    for (size_t i = 0; i != this->size(); ++i) {
      (*this)[i][worker_index].fetch_add(worker_stat[i], std::memory_order_relaxed);
    }
  }

  auto get_stat(typename E::Key key, uint16_t worker_index) const noexcept {
    return (*this)[key][worker_index].load(std::memory_order_relaxed);
  }

  void set_stat(typename E::Key key, uint16_t worker_index, typename E::StatType value) noexcept {
    (*this)[key][worker_index].store(value, std::memory_order_relaxed);
  }

  void inc_stat(typename E::Key key, uint16_t worker_index) noexcept {
    (*this)[key][worker_index].fetch_add(1, std::memory_order_relaxed);
  }
};

template<typename T>
struct WorkerSamples : vk::not_copyable {
public:
  void recalc(size_t worker_index) {
    percentiles.update_percentiles(samples.begin(), samples.begin() + worker_index);
  }

  Percentiles<T> percentiles;
  std::array<T, WorkersControl::max_workers_count> samples{};
};

template<typename E>
struct WorkerSamplesBundle : EnumTable<E, WorkerSamples<typename E::StatType>>, private vk::not_copyable {
public:
  void recalc(const WorkerStatsBundle<E> &stats, uint16_t first_worker_id, uint16_t last_worker_id) noexcept {
    if (const uint16_t len = last_worker_id - first_worker_id) {
      for (size_t i = 0; i != this->size(); ++i) {
        const auto &stat = stats[i];
        size_t buffer_index = 0;
        size_t worker_index = first_worker_id;
        while (worker_index != last_worker_id) {
          const auto sample = stat[worker_index++].load(std::memory_order_relaxed);
          if (std::is_floating_point<typename E::StatType>{} || sample != 0) {
            (*this)[i].samples[buffer_index++] = sample;
          }
        }
        (*this)[i].recalc(buffer_index);
      }
    }
  }
};

struct WorkerProcessStats : private vk::not_copyable {
  WorkerStatsBundle<MallocStat> malloc_stats{};
  WorkerStatsBundle<HeapStat> heap_stats{};
  WorkerStatsBundle<VMStat> vm_stats{};
  WorkerStatsBundle<MiscStat> misc_stats{};
  WorkerStatsBundle<QueriesStat> query_stats{};
  WorkerStatsBundle<IdleStat> idle_stats{};

  void update_worker_stats(uint16_t worker_index) noexcept {
    malloc_stats.set_worker_stats(get_malloc_stat(), worker_index);
    heap_stats.set_worker_stats(get_heap_stat(), worker_index);
    vm_stats.set_worker_stats(get_virtual_memory_stat(), worker_index);
    idle_stats.set_worker_stats(get_idle_stat(), worker_index);
    misc_stats.inc_stat(MiscStat::Key::worker_activity_counter, worker_index);
  }

  void update_worker_special_connections(uint64_t active_connections, uint64_t max_connections, uint16_t worker_index) noexcept {
    misc_stats.set_stat(MiscStat::Key::active_special_connections, worker_index, active_connections);
    misc_stats.set_stat(MiscStat::Key::max_special_connections, worker_index, max_connections);
  }

  void update_worker_status(uint64_t status, uint16_t worker_index) noexcept {
    misc_stats.set_stat(MiscStat::Key::worker_status, worker_index, status);
  }

  void add_worker_stats(const EnumTable<QueriesStat> &worker_stat, uint16_t worker_index) noexcept {
    query_stats.add_worker_stats(worker_stat, worker_index);
  }

  void reset_worker_stats(pid_t worker_pid, uint64_t active_connections, uint64_t max_connections, uint16_t worker_index) noexcept {
    query_stats.set_worker_stats(EnumTable<QueriesStat>{}, worker_index);
    misc_stats.set_stat(MiscStat::Key::worker_activity_counter, worker_index, 1);
    misc_stats.set_stat(MiscStat::Key::process_pid, worker_index, worker_pid);
    misc_stats.set_stat(MiscStat::Key::worker_status, worker_index, MiscStat::worker_idle);
    update_worker_special_connections(active_connections, max_connections, worker_index);
    update_worker_stats(worker_index);
  }
};

template<class E>
struct WorkerPercentilesBundle : EnumTable<E, Percentiles<typename E::StatType>>, private vk::not_copyable {
  void recalc(const WorkerStatsBundle<E> &stats, uint16_t first_worker_id, uint16_t last_worker_id) noexcept {
    if (const uint16_t len = last_worker_id - first_worker_id) {
      typename E::StatType buffer[len];
      for (size_t i = 0; i != this->size(); ++i) {
        const auto &stat = stats[i];
        size_t buffer_index = 0;
        size_t worker_index = first_worker_id;
        while (worker_index != last_worker_id) {
          const auto s = stat[worker_index++].load(std::memory_order_relaxed);
          if (std::is_floating_point<typename E::StatType>{} || s != 0) {
            buffer[buffer_index++] = s;
          }
        }
        (*this)[i].update_percentiles(buffer, buffer + buffer_index);
      }
    }
  }
};

struct WorkerAggregatedStats {
  explicit WorkerAggregatedStats(std::mt19937 *gen) noexcept:
    script_samples(gen) {
  }

  void recalc(SharedSamplesBundle<ScriptSamples> &script_shared_samples, std::chrono::steady_clock::time_point now_tp,
              const WorkerProcessStats &stats, uint16_t first_id, uint16_t last_id) noexcept {
    script_samples.recalc(script_shared_samples, now_tp);
    heap_samples.recalc(stats.heap_stats, first_id, last_id);
    malloc_samples.recalc(stats.malloc_stats, first_id, last_id);
    vm_samples.recalc(stats.vm_stats, first_id, last_id);
    idle_samples.recalc(stats.idle_stats, first_id, last_id);
  }

  AggregatedSamplesBundle<ScriptSamples> script_samples;
  WorkerSamplesBundle<MallocStat> malloc_samples;
  WorkerSamplesBundle<HeapStat> heap_samples;
  WorkerSamplesBundle<VMStat> vm_samples;
  WorkerSamplesBundle<IdleStat> idle_samples;
};

struct JobWorkerAggregatedStats : WorkerAggregatedStats {
  explicit JobWorkerAggregatedStats(std::mt19937 *gen) noexcept:
    WorkerAggregatedStats(gen),
    job_samples(gen),
    job_common_memory_samples(gen) {
  }

  AggregatedSamplesBundle<JobSamples> job_samples;
  AggregatedSamplesBundle<JobCommonMemorySamples> job_common_memory_samples;
};

struct MasterProcessStats : private vk::not_copyable {
  EnumTable<VMStat> vm_stats;
  EnumTable<MallocStat> malloc_stats;
  EnumTable<IdleStat> idle_stats;
};

} // namespace

struct ServerStats::SharedStats {
  explicit SharedStats(std::mt19937 *gen) noexcept:
    general_workers(gen),
    job_workers(gen) {
  }

  WorkerSharedStats general_workers;
  JobWorkerSharedStats job_workers;

  WorkerProcessStats workers;
};


struct ServerStats::AggregatedStats {
  explicit AggregatedStats(std::mt19937 *gen) noexcept:
    general_workers(gen),
    job_workers(gen) {
  }

  WorkerAggregatedStats general_workers;
  JobWorkerAggregatedStats job_workers;

  MasterProcessStats master_process;
};

void ServerStats::init() noexcept {
  gen_ = new std::mt19937{};
  aggregated_stats_ = new AggregatedStats{gen_};
  shared_stats_ = new(mmap_shared(sizeof(SharedStats))) SharedStats{gen_};
}

void ServerStats::after_fork(pid_t worker_pid, uint64_t active_connections, uint64_t max_connections,
                             uint16_t worker_process_id, WorkerType worker_type) noexcept {
  assert(vk::any_of_equal(worker_type, WorkerType::general_worker, WorkerType::job_worker));
  worker_process_id_ = worker_process_id;
  worker_type_ = worker_type;
  gen_->seed(worker_pid);
  shared_stats_->workers.reset_worker_stats(worker_pid, active_connections, max_connections, worker_process_id_);
  last_update_ = std::chrono::steady_clock::now();
}

void ServerStats::add_request_stats(double script_time_sec, double net_time_sec, int64_t script_queries, int64_t long_script_queries, int64_t memory_used,
                                    int64_t real_memory_used, int64_t curl_total_allocated, script_error_t error) noexcept {
  auto &stats = worker_type_ == WorkerType::job_worker ? shared_stats_->job_workers : shared_stats_->general_workers;
  const auto script_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(script_time_sec));
  const auto net_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(net_time_sec));
  const auto queries_stat = make_queries_stat(script_queries, long_script_queries, script_time.count(), net_time.count());

  stats.add_request_stats(queries_stat, error, memory_used, real_memory_used, curl_total_allocated);
  shared_stats_->workers.add_worker_stats(queries_stat, worker_process_id_);

  using namespace statshouse;
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::memory_used, worker_type_, memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::real_memory_used, worker_type_, real_memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::total_allocated_by_curl, worker_type_, curl_total_allocated);

  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::script_time, worker_type_, script_time.count());
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::net_time, worker_type_, net_time.count());
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::outgoing_queries, worker_type_, script_queries);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(GenericQueryStatKey::outgoing_long_queries, worker_type_, long_script_queries);
}

void ServerStats::add_job_stats(double job_wait_time_sec, int64_t request_memory_used, int64_t request_real_memory_used, int64_t response_memory_used,
                                int64_t response_real_memory_used) noexcept {
  const auto job_wait_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(job_wait_time_sec));
  shared_stats_->job_workers.add_job_stats(job_wait_time.count(), request_memory_used, request_real_memory_used, response_memory_used, response_real_memory_used);

  using namespace statshouse;
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_wait_time, job_wait_time.count());
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_request_memory_usage, request_memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_request_real_memory_usage, request_real_memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_response_memory_usage, response_memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_response_real_memory_usage, response_real_memory_used);
}

void ServerStats::add_job_common_memory_stats(int64_t common_request_memory_used, int64_t common_request_real_memory_used) noexcept {
  shared_stats_->job_workers.add_job_common_memory_stats(common_request_memory_used, common_request_real_memory_used);

  using namespace statshouse;
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_common_request_memory_usage, common_request_memory_used);
  vk::singleton<WorkerStatsBuffer>::get().add_query_stat(QueryStatKey::job_common_request_real_memory_usage, common_request_real_memory_used);
}

void ServerStats::update_this_worker_stats() noexcept {
  const auto now_tp = std::chrono::steady_clock::now();
  if (now_tp - last_update_ >= std::chrono::seconds{5}) {
    shared_stats_->workers.update_worker_stats(worker_process_id_);
    last_update_ = now_tp;
  }
}

void ServerStats::update_active_connections(uint64_t active_connections, uint64_t max_connections) noexcept {
  shared_stats_->workers.update_worker_special_connections(active_connections, max_connections, worker_process_id_);
}

void ServerStats::set_idle_worker_status() noexcept {
  shared_stats_->workers.update_worker_status(MiscStat::worker_idle, worker_process_id_);
}

void ServerStats::set_wait_net_worker_status() noexcept {
  shared_stats_->workers.update_worker_status(MiscStat::worker_waiting_net | MiscStat::worker_running, worker_process_id_);
}

void ServerStats::set_running_worker_status() noexcept {
  shared_stats_->workers.update_worker_status(MiscStat::worker_running, worker_process_id_);
}

void ServerStats::aggregate_stats() noexcept {
  const auto now_tp = std::chrono::steady_clock::now();
  if (now_tp - last_update_ < std::chrono::seconds{5}) {
    return;
  }

  last_update_ = now_tp;
  const auto &workers_control = vk::singleton<WorkersControl>::get();

  const uint16_t general_workers = workers_control.get_count(WorkerType::general_worker);
  const uint16_t job_workers = workers_control.get_count(WorkerType::job_worker);

  aggregated_stats_->general_workers.recalc(shared_stats_->general_workers.script_samples, now_tp,
                                            shared_stats_->workers, 0, general_workers);

  aggregated_stats_->job_workers.job_samples.recalc(shared_stats_->job_workers.job_samples, now_tp);
  aggregated_stats_->job_workers.job_common_memory_samples.recalc(shared_stats_->job_workers.job_common_memory_samples, now_tp);
  aggregated_stats_->job_workers.recalc(shared_stats_->job_workers.script_samples, now_tp,
                                        shared_stats_->workers, general_workers, job_workers + general_workers);

  aggregated_stats_->master_process.vm_stats = get_virtual_memory_stat();
  aggregated_stats_->master_process.malloc_stats = get_malloc_stat();
  aggregated_stats_->master_process.idle_stats = get_idle_stat();
}

namespace {

double ns2double(uint64_t ns) noexcept {
  return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::nanoseconds{static_cast<int64_t>(ns)}).count();
}

uint64_t kb2bytes(uint64_t kb) noexcept {
  return kb * 1024;
}

template<typename T, typename Mapper = vk::identity>
void write_to(stats_t *stats, const char *prefix, const char *suffix, const AggregatedSamples<T> &samples, const Mapper &mapper = {}) {
  if (stats->need_aggregated_stats()) {
    stats->add_gauge_stat(mapper(samples.percentiles.p50), prefix, suffix, ".p50");
    stats->add_gauge_stat(mapper(samples.percentiles.p95), prefix, suffix, ".p95");
    stats->add_gauge_stat(mapper(samples.percentiles.p99), prefix, suffix, ".p99");
    stats->add_gauge_stat(mapper(samples.percentiles.max), prefix, suffix, ".max");
  }
}

template<typename T, typename Mapper = vk::identity>
void write_to(stats_t *stats, const char *prefix, const char *suffix, const WorkerSamples<T> &samples, const Mapper &mapper = {}) {
  if (stats->need_aggregated_stats()) {
    stats->add_gauge_stat(mapper(samples.percentiles.p50), prefix, suffix, ".p50");
    stats->add_gauge_stat(mapper(samples.percentiles.p95), prefix, suffix, ".p95");
    stats->add_gauge_stat(mapper(samples.percentiles.p99), prefix, suffix, ".p99");
    stats->add_gauge_stat(mapper(samples.percentiles.max), prefix, suffix, ".max");
  } else {
    const uint16_t workers_count = vk::singleton<WorkersControl>::get().get_total_workers_count();
    std::vector<double> values(samples.samples.begin(), samples.samples.begin() + workers_count);
    stats->add_multiple_gauge_stats(std::move(values), prefix, suffix);
  }
}

void write_to(stats_t *stats, const char *prefix, const WorkerAggregatedStats &agg, const WorkerSharedStats &shared) noexcept {
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::memory_limit)], prefix, ".errors.memory_limit_exceeded");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::timeout)], prefix, ".errors.timeout");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::exception)], prefix, ".errors.exception");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::stack_overflow)], prefix, ".errors.stack_overflow");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::php_assert)], prefix, ".errors.php_assert");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::http_connection_close)], prefix, ".errors.http_connection_close");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::rpc_connection_close)], prefix, ".errors.rpc_connection_close");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::net_event_error)], prefix, ".errors.net_event_error");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::post_data_loading_error)], prefix, ".errors.post_data_loading_error");
  stats->add_gauge_stat(shared.errors[static_cast<size_t>(script_error_t::unclassified_error)], prefix, ".errors.unclassified");

  stats->add_gauge_stat(ns2double(shared.total_queries_stat[QueriesStat::Key::script_time]), prefix, ".requests.script_time.total");
  stats->add_gauge_stat(ns2double(shared.total_queries_stat[QueriesStat::Key::net_time]), prefix, ".requests.net_time.total");
  stats->add_gauge_stat(shared.total_queries_stat[QueriesStat::Key::incoming_queries], prefix, ".requests.total_incoming_queries");
  stats->add_gauge_stat(shared.total_queries_stat[QueriesStat::Key::outgoing_queries], prefix, ".requests.total_outgoing_queries");
  stats->add_gauge_stat(shared.total_queries_stat[QueriesStat::Key::outgoing_long_queries], prefix, ".requests.total_outgoing_long_queries");

  write_to(stats, prefix, ".requests.outgoing_queries", agg.script_samples[ScriptSamples::Key::outgoing_queries]);
  write_to(stats, prefix, ".requests.outgoing_long_queries", agg.script_samples[ScriptSamples::Key::outgoing_long_queries]);
  write_to(stats, prefix, ".requests.script_time", agg.script_samples[ScriptSamples::Key::script_time], ns2double);
  write_to(stats, prefix, ".requests.net_time", agg.script_samples[ScriptSamples::Key::net_time], ns2double);
  write_to(stats, prefix, ".requests.working_time", agg.script_samples[ScriptSamples::Key::working_time], ns2double);
  write_to(stats, prefix, ".memory.script_usage", agg.script_samples[ScriptSamples::Key::memory_used]);
  write_to(stats, prefix, ".memory.script_real_usage", agg.script_samples[ScriptSamples::Key::real_memory_used]);
  write_to(stats, prefix, ".memory.script_total_allocated_by_curl", agg.script_samples[ScriptSamples::Key::total_allocated_by_curl]);

  write_to(stats, prefix, ".memory.currently_script_heap_usage_bytes", agg.heap_samples[HeapStat::Key::script_heap_memory_usage]);
  write_to(stats, prefix, ".memory.currently_allocated_by_curl_bytes", agg.heap_samples[HeapStat::Key::curl_memory_currently_usage]);

  write_to(stats, prefix, ".memory.malloc_non_mapped_allocated_bytes", agg.malloc_samples[MallocStat::Key::non_mmaped_allocated_bytes]);
  write_to(stats, prefix, ".memory.malloc_non_mapped_free_bytes", agg.malloc_samples[MallocStat::Key::non_mmaped_free_bytes]);
  write_to(stats, prefix, ".memory.malloc_mapped_bytes", agg.malloc_samples[MallocStat::Key::mmaped_bytes]);

  write_to(stats, prefix, ".memory.rss_bytes", agg.vm_samples[VMStat::Key::rss_kb], kb2bytes);
  write_to(stats, prefix, ".memory.vms_bytes", agg.vm_samples[VMStat::Key::vm_kb], kb2bytes);
  write_to(stats, prefix, ".memory.shm_bytes", agg.vm_samples[VMStat::Key::shm_kb], kb2bytes);

  write_to(stats, prefix, ".cpu.recent_idle", agg.idle_samples[IdleStat::Key::recent_idle_percent]);
}

void write_to(stats_t *stats, const char *prefix, const JobWorkerAggregatedStats &job_agg) noexcept {
  write_to(stats, prefix, ".jobs.queue_time", job_agg.job_samples[JobSamples::Key::wait_time], ns2double);
  write_to(stats, prefix, ".memory.job_request_usage", job_agg.job_samples[JobSamples::Key::request_memory_usage]);
  write_to(stats, prefix, ".memory.job_request_real_usage", job_agg.job_samples[JobSamples::Key::request_real_memory_usage]);
  write_to(stats, prefix, ".memory.job_response_usage", job_agg.job_samples[JobSamples::Key::response_memory_usage]);
  write_to(stats, prefix, ".memory.job_response_real_usage", job_agg.job_samples[JobSamples::Key::response_real_memory_usage]);
  write_to(stats, prefix, ".memory.job_common_request_usage", job_agg.job_common_memory_samples[JobCommonMemorySamples::Key::common_request_memory_usage]);
  write_to(stats, prefix, ".memory.job_common_request_real_usage", job_agg.job_common_memory_samples[JobCommonMemorySamples::Key::common_request_real_memory_usage]);
}

void write_to(stats_t *stats, const char *prefix, const MasterProcessStats &master_process) noexcept {
  stats->add_gauge_stat(master_process.malloc_stats[MallocStat::Key::non_mmaped_allocated_bytes], prefix, ".memory.malloc_non_mapped_allocated_bytes");
  stats->add_gauge_stat(master_process.malloc_stats[MallocStat::Key::non_mmaped_free_bytes], prefix, ".memory.malloc_non_mapped_free_bytes");
  stats->add_gauge_stat(master_process.malloc_stats[MallocStat::Key::mmaped_bytes], prefix, ".memory.malloc_mapped_bytes");

  stats->add_gauge_stat(kb2bytes(master_process.vm_stats[VMStat::Key::rss_kb]), prefix, ".memory.rss_bytes");
  stats->add_gauge_stat(kb2bytes(master_process.vm_stats[VMStat::Key::vm_kb]), prefix, ".memory.vms_bytes");
  stats->add_gauge_stat(kb2bytes(master_process.vm_stats[VMStat::Key::shm_kb]), prefix, ".memory.shm_bytes");
}

template<class S>
auto get_max(const WorkerSamplesBundle<S> &general_stats, const WorkerSamplesBundle<S> &job_stats,
             const EnumTable<S> &master_stats, typename S::Key key) noexcept {
  return std::max(master_stats[key], std::max(general_stats[key].percentiles.max, job_stats[key].percentiles.max));
}

template<class S>
auto get_sum(const WorkerSamplesBundle<S> &general_stats, const WorkerSamplesBundle<S> &job_stats,
             const EnumTable<S> &master_stats, typename S::Key key) noexcept {
  return master_stats[key] + general_stats[key].percentiles.sum + job_stats[key].percentiles.sum;
}

void write_server_vm_to(stats_t *stats, const char *prefix, const EnumTable<VMStat> &master_vm,
                        const WorkerSamplesBundle<VMStat> &general_vm, const WorkerSamplesBundle<VMStat> &job_vm) noexcept {
  stats->add_gauge_stat(kb2bytes(get_max(general_vm, job_vm, master_vm, VMStat::Key::vm_peak_kb)), prefix, ".memory.vms_max_bytes");
  stats->add_gauge_stat(kb2bytes(get_max(general_vm, job_vm, master_vm, VMStat::Key::rss_peak_kb)), prefix, ".memory.rss_max_bytes");
  stats->add_gauge_stat(kb2bytes(get_max(general_vm, job_vm, master_vm, VMStat::Key::shm_kb)), prefix, ".memory.shm_max_bytes");

  const uint64_t rss_no_shm = get_sum(general_vm, job_vm, master_vm, VMStat::Key::rss_kb) - get_sum(general_vm, job_vm, master_vm, VMStat::Key::shm_kb);
  stats->add_gauge_stat(kb2bytes(rss_no_shm), prefix, ".memory.rss_no_shm_total_bytes");
}

} // namespace

void ServerStats::write_stats_to(stats_t *stats) const noexcept {
  write_to(stats, "workers.general", aggregated_stats_->general_workers, shared_stats_->general_workers);

  write_to(stats, "workers.job", aggregated_stats_->job_workers, shared_stats_->job_workers);
  write_to(stats, "workers.job", aggregated_stats_->job_workers);

  write_to(stats, "master", aggregated_stats_->master_process);

  write_server_vm_to(stats, "server", aggregated_stats_->master_process.vm_stats,
                     aggregated_stats_->general_workers.vm_samples, aggregated_stats_->job_workers.vm_samples);
}

void ServerStats::write_stats_to(std::ostream &os, bool add_worker_pids) const noexcept {
  const auto &master_vm = aggregated_stats_->master_process.vm_stats;
  const auto &master_idle = aggregated_stats_->master_process.idle_stats;

  const auto &general_vm = aggregated_stats_->general_workers.vm_samples;
  const auto &general_idle = aggregated_stats_->general_workers.idle_samples;

  const auto &job_vm = aggregated_stats_->job_workers.vm_samples;
  const auto &job_idle = aggregated_stats_->job_workers.idle_samples;

  const auto &total_queries = shared_stats_->general_workers.total_queries_stat;
  const uint16_t workers_count = vk::singleton<WorkersControl>::get().get_total_workers_count();

  const auto total_net_time = ns2double(total_queries[QueriesStat::Key::net_time]);
  const auto total_script_time = ns2double(total_queries[QueriesStat::Key::script_time]);

  os << std::fixed << std::setprecision(3)
     << "VM\t" << get_sum(general_vm, job_vm, master_vm, VMStat::Key::vm_kb) << "Kb\n"
     << "VM_max\t" << get_max(general_vm, job_vm, master_vm, VMStat::Key::vm_peak_kb) << "Kb\n"
     << "RSS\t" << get_sum(general_vm, job_vm, master_vm, VMStat::Key::rss_kb) << "Kb\n"
     << "RSS_max\t" << get_sum(general_vm, job_vm, master_vm, VMStat::Key::rss_peak_kb) << "Kb\n"
     << "tot_queries\t" << total_queries[QueriesStat::Key::incoming_queries].load(std::memory_order_relaxed) << "\n"
     << "tot_script_queries\t" << total_queries[QueriesStat::Key::outgoing_queries].load(std::memory_order_relaxed) << "\n"
     << "worked_time\t" << total_script_time + total_net_time << "\n"
     << "script_time\t" << total_script_time << "\n"
     << "net_time\t" << total_net_time << "\n"
     << "tot_idle_time\t" << get_sum(general_idle, job_idle, master_idle, IdleStat::Key::tot_idle_time) << "\n"
     << "tot_idle_percent\t" << get_sum(general_idle, job_idle, master_idle, IdleStat::Key::tot_idle_percent) / (1 + workers_count) << "\n"
     << "recent_idle_percent\t" << get_sum(general_idle, job_idle, master_idle, IdleStat::Key::recent_idle_percent) / (1 + workers_count) << "\n";

  const auto &workers_vm = shared_stats_->workers.vm_stats;
  const auto &workers_query = shared_stats_->workers.query_stats;
  const auto &workers_misc = shared_stats_->workers.misc_stats;
  const auto &workers_idle = shared_stats_->workers.idle_stats;
  for (uint16_t w = 0; w != workers_count; ++w) {
    const auto net_time = ns2double(workers_query.get_stat(QueriesStat::Key::net_time, w));
    const auto script_time = ns2double(workers_query.get_stat(QueriesStat::Key::script_time, w));
    const auto worker_pid = workers_misc.get_stat(MiscStat::Key::process_pid, w);
    if (add_worker_pids) {
      os << "pid " << worker_pid << "\t" << worker_pid << "\n";
    }
    os << "active_special_connections " << worker_pid << "\t" << workers_misc.get_stat(MiscStat::Key::active_special_connections, w) << "\n"
       << "max_special_connections " << worker_pid << "\t" << workers_misc.get_stat(MiscStat::Key::max_special_connections, w) << "\n"
       << "VM " << worker_pid << "\t" << workers_vm.get_stat(VMStat::Key::vm_kb, w) << "Kb\n"
       << "VM_max " << worker_pid << "\t" << workers_vm.get_stat(VMStat::Key::vm_peak_kb, w) << "Kb\n"
       << "RSS " << worker_pid << "\t" << workers_vm.get_stat(VMStat::Key::rss_kb, w) << "Kb\n"
       << "RSS_max " << worker_pid << "\t" << workers_vm.get_stat(VMStat::Key::rss_peak_kb, w) << "Kb\n"
       << "tot_queries " << worker_pid << "\t" << workers_query.get_stat(QueriesStat::Key::incoming_queries, w) << "\n"
       << "tot_script_queries " << worker_pid << "\t" << workers_query.get_stat(QueriesStat::Key::outgoing_queries, w) << "\n"
       << "worked_time " << worker_pid << "\t" << net_time + script_time << "\n"
       << "script_time " << worker_pid << "\t" << script_time << "\n"
       << "net_time " << worker_pid << "\t" << net_time << "\n"
       << "tot_idle_time " << worker_pid << "\t" << workers_idle.get_stat(IdleStat::Key::tot_idle_time, w) << "\n"
       << "tot_idle_percent " << worker_pid << "\t" << workers_idle.get_stat(IdleStat::Key::tot_idle_percent, w) << "\n"
       << "recent_idle_percent " << worker_pid << "\t" << workers_idle.get_stat(IdleStat::Key::recent_idle_percent, w) << "\n";
  }
}

ServerStats::WorkersStat ServerStats::collect_workers_stat(WorkerType worker_type) const noexcept {
  assert(vk::any_of_equal(worker_type, WorkerType::general_worker, WorkerType::job_worker));

  const auto &workers_control = vk::singleton<WorkersControl>::get();
  const uint16_t general_workers = workers_control.get_count(WorkerType::general_worker);
  const uint16_t job_workers = workers_control.get_count(WorkerType::job_worker);

  const uint16_t first = worker_type == WorkerType::general_worker ? 0 : general_workers;
  const uint16_t last = worker_type == WorkerType::general_worker ? general_workers : job_workers + general_workers;
  WorkersStat result;
  const auto &workers_misc = shared_stats_->workers.misc_stats;
  for (uint16_t w = first; w != last; ++w) {
    const auto worker_status = workers_misc.get_stat(MiscStat::Key::worker_status, w);
    result.running_workers += (worker_status & MiscStat::worker_running) ? 1 : 0;
    result.waiting_workers += (worker_status & MiscStat::worker_waiting_net) ? 1 : 0;
    const auto max_connections = workers_misc.get_stat(MiscStat::Key::max_special_connections, w);
    const auto active_connections = workers_misc.get_stat(MiscStat::Key::active_special_connections, w);
    result.ready_for_accept_workers += (active_connections != max_connections) ? 1 : 0;
  }
  result.total_workers = last - first;
  return result;
}

uint64_t ServerStats::get_worker_activity_counter(uint16_t worker_process_id) const noexcept {
  return shared_stats_->workers.misc_stats.get_stat(MiscStat::Key::worker_activity_counter, worker_process_id);
}
