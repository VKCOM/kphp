---
sort: 2
---

# KPHP functions extending PHP

Here we list functions, that extend PHP standard library and are available in KPHP natively.

```tip
Skip this page at first reading. Look through it later, when you start using KPHP.
```


## Type system

<aside>tuple(...args)</aside>
<aside>shape([assoc])</aside>
<aside>not_null(any)</aside>
<aside>not_false(any)</aside>

These are keywords (language constructs), described [in the type system article](../static-type-system/kphp-type-system.md).

<aside>instance_cast(object $instance, string $class_name): \$class_name</aside>
<aside>to_array_debug(object $instance): mixed[]</aside>

Functions for casting instances, described [here](../static-type-system/type-casting.md).


## arrays

<aside>array_first_key(T[] $a): int|string|null</aside>
<aside>array_first_value(T[] $a): T</aside>
<aside>array_last_key(T[] $a): int|string|null</aside>
<aside>array_last_value(T[] $a): T</aside>

Since KPHP doesn't support internal array pointers and functions like `end()`, it's a typed alternative.

<aside>array_keys_as_strings(T[] $a): string[]</aside>
<aside>array_keys_as_ints(T[] $a): int[]</aside>
Since PHP arrays can be keyed with both integers and strings, `array_keys()` returns `mixed[]`.  

But if you know, that your particular array carries only one type of keys, use these functions with clean types.

<aside>array_swap_int_keys(array &$a, int $idx1, int $idx2): void</aside>

Swaps two elements in the array, like `tmp = a[idx1]; a[idx1] = a[idx2]; a[idx2] = tmp`, but more effeciently, especially for vectors.

<aside>array_merge_into(T[] &$dest, T[] $another): void</aside>

Same as `$dest = array_merge($dest, $another)`, but without creating a temporary array.

<aside>array_find(T[] $a, callable(T):bool $callback): tuple(int|string|null, T)</aside>

Find an element in an array with callback.  
tuple[0] is corresponding key (*null* if not exists).  
tuple[1] is a found value (empty *T* if not exists).

<aside>array_filter_by_key(T[] $array, callable(T):bool $callback): T[]</aside>

Since `array_filter()` in KPHP doesn't accept `ARRAY_FILTER_USE_KEY`, it's an alternative.  
(as support of *ARRAY_FILTER_USE_KEY* can't be done without callback argument type spoiling)

<aside>array_reserve(T[] &$arr, int $int_keys_num, int $str_keys_num, bool $is_vector): void</aside>

Reserve array memory — if you know in advance, how many elements will be inserted to it.  
Effectife especially for vectors, as there will be no reallocations on insertion.  
*$arr* — target array (typically empty)  
*$int_keys_num* — number of int keys  
*$str_keys_num* — number of string keys  
*$is_vector* — should it be a vector (if *$str_keys_num* is 0)

<aside>array_reserve_from(T[] &$arr, T2[] $base): void</aside>

The same as `array_reserve()`, but takes all sizes (length, key type, is vector) from array *$base*.


## Async programming

<aside>fork( f(): T ): future&lt;T&gt;</aside>
<aside>wait( future&lt;T&gt;|false ): ?T (resumable)</aside>
<aside>... and others</aside>  

Read about [coroutines (forks)](../best-practices/async-programming-forks.md).


## RPC calls

<aside>rpc_tl_query($connection, $queries, $timeout = -1.0, $ignore_result = false): mixed[]</aside>
<aside>fetch_int(): int</aside>
<aside>... and others</aside>

Read about [TL schema and RPC calls](../../kphp-client).


## Shared memory (instance cache)

<aside>instance_cache_fetch(string $type, string $key, bool $even_if_expired = false) : ?\$type</aside>
<aside>instance_cache_store(string $key, object $value, int $ttl): bool</aside>
<aside>... and others</aside>

Read about [shared memory](../best-practices/shared-memory.md).


## Memory stats

<aside>get_global_vars_memory_stats(): int[]</aside>

Returns an array `["$var_name" => size_in_bytes]`  
Don't use it in production — only to debug, which globals/statics allocate huge pieces of memory.  
While compiling, a special env variable should be set: `KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS=1`

<aside>memory_get_total_usage(): int</aside>

Returns currently used and dirty memory (in bytes).

<aside>memory_get_static_usage(): int</aside>

Returns heap memory usage (system heap, not script allocator) (in bytes).


## Serialization to binary format

<aside>instance_serialize(object $instance): ?string</aside>
<aside>instance_deserialize(string $packed_str, string $type): ?\$type</aside>
<aside>msgpack_serialize(mixed $value): string</aside>
<aside>msgpack_deserialize(string $packed_str): mixed</aside>

Read about [serialization and msgpack](../howto-by-kphp/serialization-msgpack.md).


## Profiling

<aside>profiler_is_enabled(): bool</aside>
<aside>profiler_set_function_label(string $label): void</aside>
<aside>profiler_set_log_suffix(string $suffix): void</aside>

Read about [embedded profiler](../best-practices/embedded-profiler.md).


## Misc

<aside>warning(string $str): void</aside>

Same as PHP `trigger_error($str, E_USER_WARNING);` (KPHP doesn't have a *trigger_error()* function).

<aside>register_kphp_on_warning_callback(callable(string, string[]) $callback): void</aside>

Useful for dev purposes: this callback is invoked when a runtime warning occurs.  
It can be shown on screen for the developer, for example.  
Do not use it in production! Use json log analyzer and trace C++ → PHP mapper [instead](../../kphp-server/deploy-and-maintain/map-cpp-trace-to-php.md).  
*$callback* is invoked with 2 arguments: 1) *$message*: warning text; 2) *$stacktrace*: function names demangled from cpp trace, even without debug symbols — but slow, only for dev

<aside>kphp_backtrace(): string[]</aside>

Like *register_kphp_on_warning_callback()*, but it is not linked to any runtime error: instead, it allows getting current demangled backtrace at the execution point.  
Note! Demangling works slowly, don't use it in high-loaded places!

<aside>kphp_set_context_on_error(string[] $tags, mixed[] $extra_info, string $env = ''): void</aside>

Defines a context for runtime warnings (to be written to [json error logs](../../kphp-server/deploy-and-maintain/logging.md#json-logs)).    
*$tags* — key-value tags (treated like an aggregate)  
*$extra_info* — key-value extra arbitrary data (not an aggregator)  
*$env* — environment (e.g.: staging / production)

<aside>likely(bool $value): bool</aside>
<aside>unlikely(bool $value): bool</aside>

Special intrinsic to wrap conditions inside *if*, like `if (likely($x > 10))` — when you suppose that condition is much likely to happen than not (*unlikely()* is vice versa).  
This can slightly pre-warm the CPU branch predictor.


```note
All these functions work in plain PHP also — they are **polyfilled** to behave exactly like KPHP's built-in. 
```
