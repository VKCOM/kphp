---
sort: 3
---

# Runtime metrics and statsd

KPHP server collects various metrics and pushes them into statsd service. This feature can be configured with the following options: `--statsd-host`, `--statsd-port`, `--disable-statsd`.  

The master process collects, aggregates, and pushes metrics every second. By default, it tries to connect to the **8125** or **14880** port.
Once the KPHP server connects to any port, it stops trying. In case of connection lost, the master process tries to reconnect.

Here it is the list of all available metrics:

### 1. General stats

* _kphp_server.kphp_version_ — the version of php code. Specified by `--error-tag/-E` option; 
* _kphp_server.uptime_ — seconds passed from the KPHP server start;
* _kphp_server.cpu_utime_ — master + workers CPU time in user mode (check **/proc/[pid]/stat (14)**);
* _kphp_server.cpu_stime_ — master + workers CPU time in kernel mode (check **/proc/[pid]/stat (15)**);

### 2. Workers stats

* _kphp_server.workers_total_started_ — total number of started workers;
* _kphp_server.workers_total_dead_ — total number of dead workers;
* _kphp_server.workers_total_strange_dead_ — total number of dead workers by strange reasons (SIGSEGV, etc);
* _kphp_server.workers_total_hung_ — total number of hung workers;
* _kphp_server.workers_total_killed_ — total number of hung workers killed by the master process;
* _kphp_server.workers_total_terminated_ — total number of workers terminated by the master process;
* _kphp_server.workers_total_failed_ — total number of troubles on worker termination;
* _kphp_server.workers_current_total_ — number of available workers;
* _kphp_server.workers_current_working_ — number of working workers;
* _kphp_server.workers_current_working_but_waiting_ — number of workers that work but currently are waiting for network response;
* _kphp_server.workers_current_ready_for_accept_ — number of workers ready to accept a new tcp connection;
* _kphp_server.workers_running_avg_1m_ — average number of working workers for the last minute;
* _kphp_server.workers_running_max_1m_ — maximum number of working workers for the last minute;

### 3. Requests stats

* _kphp_server.requests_total_incoming_queries_ — total number of incoming queries;
* _kphp_server.requests_total_outgoing_queries_ — total number of outgoing (to databases) queries;
* _kphp_server.requests_script_time_total_ — total number of time (seconds) in php code;
* _kphp_server.requests_script_time_percentile_50_ — request php code time, 50th percentile; 
* _kphp_server.requests_script_time_percentile_95_ — request php code time, 95th percentile;
* _kphp_server.requests_script_time_percentile_99_ — request php code time, 99th percentile;
* _kphp_server.requests_net_time_total_ — total number of time (seconds) in network awaiting (databases);
* _kphp_server.requests_net_time_percentile_50_ — request net time, 50th percentile; 
* _kphp_server.requests_net_time_percentile_95_ — request net time, 95th percentile; 
* _kphp_server.requests_net_time_percentile_99_ — request net time, 99th percentile; 
* _kphp_server.requests_working_time_percentile_50_ — request full time, 50th percentile;
* _kphp_server.requests_working_time_percentile_95_ — request full time, 95th percentile;
* _kphp_server.requests_working_time_percentile_99_ — request full time, 99th percentile;
* _kphp_server.requests_incoming_queries_per_second_ — requests incoming QPS;
* _kphp_server.requests_outgoing_queries_per_second_ — requests outgoing QPS (to databases);

### 4. Terminated requests stats

* _kphp_server.terminated_requests_timeout_ — total number of terminations due to server timeout;
* _kphp_server.terminated_requests_http_connection_close_ — total number of terminations due to closed HTTP connection;
* _kphp_server.terminated_requests_rpc_connection_close_ — total number of terminations due to closed RPC connection;
* _kphp_server.terminated_requests_memory_limit_exceeded_ — total number of terminations due to memory limit exceeded;
* _kphp_server.terminated_requests_exception_ — total number of terminations due to uncaught exceptions;
* _kphp_server.terminated_requests_stack_overflow_ — total number of terminations due to stack overflow;
* _kphp_server.terminated_requests_php_assert_ — total number of terminations due to assertion in KPHP runtime or server code;
* _kphp_server.terminated_requests_net_event_error_ — total number of terminations due to network errors;
* _kphp_server.terminated_requests_post_data_loading_error_ — total number of terminations due to POST body receiving failure;
* _kphp_server.terminated_requests_unclassified_ — total number of terminations due to unclassified reason;

### 5. Memory stats

* _kphp_server.memory_script_usage_max_ — request memory usage maximum (since start);
* _kphp_server.memory_script_usage_percentile_50_ — request memory usage 50th percentile;
* _kphp_server.memory_script_usage_percentile_95_ — request memory usage 95th percentile;
* _kphp_server.memory_script_usage_percentile_99_ — request memory usage 99th percentile;
* _kphp_server.memory_script_real_usage_max_ — request allocator memory usage maximum;
* _kphp_server.memory_script_real_usage_percentile_50_ — request allocator memory usage 50th percentile;
* _kphp_server.memory_script_real_usage_percentile_95_ — request allocator memory usage 95th percentile;
* _kphp_server.memory_script_real_usage_percentile_99_ — request allocator memory usage 99th percentile;
* _kphp_server.memory_vms_max_ — maximum vms usage by a single worker;
* _kphp_server.memory_rss_max_ — maximum rss usage by a single worker;
* _kphp_server.memory_shared_max_ — maximum shared memory usage;

### 6. Instance cache memory

* _kphp_server.instance_cache_memory_limit_ — memory limit;
* _kphp_server.instance_cache_memory_used_ — memory usage;
* _kphp_server.instance_cache_memory_used_max_ — peak memory usage;
* _kphp_server.instance_cache_memory_real_used_ — allocator memory usage;
* _kphp_server.instance_cache_memory_real_used_max_ — allocator peak memory usage;
* _kphp_server.instance_cache_memory_defragmentation_calls_ — allocator defragmentation calls count;  
* _kphp_server.instance_cache_memory_huge_memory_pieces_ — allocator huge memory pieces count;
* _kphp_server.instance_cache_memory_small_memory_pieces_ — allocator small memory pieces count;
* _kphp_server.instance_cache_memory_buffer_swaps_ok_ — allocator buffer successful swaps count;
* _kphp_server.instance_cache_memory_buffer_swaps_fail_ — allocator buffer unsuccessful (due to worker usage) swaps count;

### 7. Instance cache elements

* _kphp_server.instance_cache_elements_stored_ — total number of elements stored to shared memory;
* _kphp_server.instance_cache_elements_stored_with_delay_ — total number of elements stored with delay (due to allocator lock);
* _kphp_server.instance_cache_elements_storing_skipped_due_recent_update_ — total number of skipped storing operations due to a recent storing from another worker;
* _kphp_server.instance_cache_elements_storing_delayed_due_mutex_ — total number of delayed storing operations due to allocator lock; 
* _kphp_server.instance_cache_elements_fetched_ — total number of fetched elements;
* _kphp_server.instance_cache_elements_missed_ — total number of missed (not found) elements;
* _kphp_server.instance_cache_elements_missed_earlier_ — total number of missed in advance elements;
* _kphp_server.instance_cache_elements_expired_ — total number of expired elements;
* _kphp_server.instance_cache_elements_created_ — total number of created elements;
* _kphp_server.instance_cache_elements_destroyed_ — total number of destroyed elements;
* _kphp_server.instance_cache_elements_cached_ — total number of elements in cache;
* _kphp_server.instance_cache_elements_logically_expired_and_ignored_ — total number of logically expired elements and ignored on fetch;
* _kphp_server.instance_cache_elements_logically_expired_but_fetched_ — total number of logically expired elements but fetched;


```tip
All these metrics are supposed to be monitored with grafana.
```
  
Outputting **kphp_version** and **uptime** shows you points of restarts.  
Checking for **workers' stats** and **memory stats** helps you determine whether a server has extra resources or its capacity is on the brink.  
Analyzing **requests stats** gives you a summary of how long users are waiting for the response.  
Observing **terminated requests** allows you to diagnose problems in PHP code.  
If your code uses [shared memory](../../kphp-language/best-practices/shared-memory.md), **instance cache stats** help you ensure, that PHP code uses it correctly without useless reallocations or unconstant keys.     
