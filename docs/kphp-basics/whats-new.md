---
sort: 7
noprevnext: true
title: What's new?
---

# What's new? (Versions history)


## 2020-11-05

* SIGTERM invokes graceful shutdown
* dropped polyfills from the repo, use Composer
* now `$a->b` and `$a[$i]` are supported inside double quotes and HEREDOC


## 2020-10-13

* *composer.json* and PSR-4 autoload
* added `gzdeflate()` and `gzinflate()`


## 2020-10-01

* Uber H3 support in KPHP and vkext
* CMake build
* added `openssl_pkcs7_sign()`
* *fixed* profiler and resumable functions called from different contexts
* profiler reports from different workers can be merged with QCacheGrind


## 2020-09-04

* typed callables
* an embedded profiler
* *fixed* template functions and instances in non-template params
* int64 in Protobuf


## 2020-08-11

* all functions must be typed if `KPHP_REQUIRE_FUNCTIONS_TYPING`
* `mt_rand()` uses Mersenne Twister
* added `filectime()`, `filemtime()`
* added `kphp_backtrace()`


## 2020-07-29

* KPHP is now 64-bit
* `func_get_args()` not supported anymore


## 2020-06-23

* if a field has no `@var`, use its default value for inferring
* all class fields must be typed if `KPHP_REQUIRE_CLASS_TYPING`
* ?: and ?? correctly infer instances without PHPDocs


## 2020-06-12

* smart casts, `not_null()`, `not_false()`
* added `array_filter_by_key()`
* a pretty table after compilation with a big verbosity
* more *CURLINFO_* options
* added `hash_equals()`
* reduces compiled binary size
* automatically inline simple functions
* instance fields inited by an empty string or an empty array use default C++ constructors


## 2020-05-22

* multiple implements, extends and implements simultaneously
* less limitations `__construct()` and extends
* `?T` in PHPDoc 
* instance cache optimizations: fewer locks in the allocator
* codegeneration and TypeData optimizations
* serializable classes now can implement interfaces
* don't generate const references for primitives
* `@kphp-should-not-throw` now outputs a whole chain, why a functions throws


## 2020-05-07

* HTTP/2 and gRPC client


## 2020-04-14

* mix lambdas and interfaces with `__invoke()`
* `float` type in TL
* *CURLOPT_RESOLVE* support
* *fixed* various hangs due to multithreading


## 2020-03-25

* shapes: named tuples
* binary serialization, msgpack, `@kphp-serializable`
* more compilation errors on strange type casts


## 2020-03-04

* added `array_find()`
* removed `/*:=...*/` syntax: use PHPDoc
* named bits in typed tl with `fields_mask.N?%True` syntax
* JSON error logs
* NAN support


## 2020-02-20

* PHP 7 support
* options for `ini_get()` with a file, not just command-line
* auth basic support
* automatic allocator defragmentation on memory limit
* SIMD number to string algorithm


## 2020-02-02

* `@kphp-should-not-throw` annotation
* `array_is_vector()` + `@kphp-flatten` + `const array<T>&` pattern analyzing


## 2020-01-23

* *false* can not be assigned to instances from now
* KPHP as an RPC server (microservices)
* support `new static`
* *fastmod* and arrays internals
* `@kphp-flatten` annotation
* `instance_cache_update_ttl()` to more accurate shared memory defragmentation control


## 2019-12-17

* PHPDoc for local and global variables, supports variables splitting


## 2019-12-04

* added `curl_multi_...()` functions
* added `array_merge_into()`
* `@kphp-warn-unused-result` annotation
* const references in read-only parameters in C++ codegeneration


## 2019-11-12

* *null* supported with instances
* optimized interface/abstract calls dispatching


## 2019-10-31

* `Optional<T>` = *T\|false\|null* (before, there was an *OrFalse<T>*, but *null* leaded to *mixed*)


## 2019-10-16

* changes in behavior to seem more like PHP 7
* spaces inside tuples and other complex PHPDoc types
* added `register_kphp_on_warning_callback()`
* added `get_global_vars_memory_stats()`


## 2019-09-26

* traits, with minor limitations
* added `array_replace()`
* *array_map('intval', $arr)* now *int[]*, not *mixed[]*


## 2019-09-11

* typed RPC (about a year of development)
* final / abstract
* indexfile and codegeneration speedup
* more metrics to grafana
* added `array_keys_as_ints()`, `array_keys_as_strings()` as typed `array_keys()`


## 2019-08-29

* shared memory between workers (instance cache)
* fixes around extends


## 2019-08-16

* extends, with the main limitation: either extends or (one) implements
* `array` type hint
* added `array_first_key()` and similar
* added `array_reserve_from()`
* added `wait_multi()` for an array of resumables
* spaceship operator `<=>`
* pack and unpack support *J/P/Q* formats
* supported *PHP_QUERY_RFC3986* in *http_build_query()* 
* *fixed* PHPDoc and variadic args
* various bugfixes and improvements


## 2019-06-19

* `fork()` returns `future<T>` (before this update it was *mixed*)
* instances and tuples can be returned from forks
* wait_queue is `future_queue<T>`
* `@kphp-infer` annotations above classes (like it's above all methods)
* empty classes optimizations
* added `openssl_sign()`, `headers_list()`
* added HEAD HTTP method


## 2019-06-06

* `instance_to_array()`
* smart `instanceof`
* `__FUNCTION__` and others
* script allocator refactoring
* `require __DIR__ . '/some.php'` now works
* `parse_url()` works like in PHP 7
* deny changing fields/methods visibility on inheritance


## 2019-04-23

* Interfaces enhancements
* `@kphp-immutable-class` annotation
* `@kphp-const` annotation
* a progress bar while compilation
* for `array_pop()` and others, no PHPDoc required for instances
* network refactoring


## 2019-04-02

* interface / implements, with the current limitation one implements only
* instance cache inside a worker across scripts invocations (but not across workers)
* TL codegeneration — all types and functions are stored/fetched by codegenerated C++ internal classes
* `final` keyword supported


## 2019-03-08

* optimized globals and statics vars reset between script reinitializations


## 2019-02-21

* variadic functions `...$args`
* patched distcc with precompiled headers (not open-sourced yet)
* `clone` keyword support with `__clone()` magic method
* ddded `sched_yield_sleep()`
* PHPDoc of static fields is now also analyzed
* `@kphp-return` tag for template functions
* `void` functions support (before this update, they were `mixed` with *null* inside)


## 2019-01-29

* *@kphp-infer* analyzes *@return*, not only *@param*
* tuples supported in PHPDoc (without spaces currently)
* *T\|bool* in PHPDoc is denied: probably you want *T\|false*
* callables can accept a string with a function name
* ddded `openssl_get_cipher_methods()`, `openssl_cipher_iv_length()`, `openssl_encrypt()`, `openssl_decrypt()`
* support several classes in a file (non-autoloadable classes can be created only within the declaring file)
* *@kphp-inline* functions can now have static variables
* deny equal *use* statements at the top of the file
* `abs()` function is correctly typed, not *mixed*
* various fixes and improvements, as usual


## 2018-12-04

* `use` inside lambdas (only by value)
* added `array_diff_assoc()`, `array_intersect_assoc()`, `array_column()`
* check all static fields to either have a default value or be inferred as *mixed*
* check all callbacks passed to built-in functions return a correct type and be exceptionless
* more correct stack traces accessed from forks
* power operator `**` support


## 2018-10-24

* `callable` keyword support as function arguments
* lambdas support (without capturing by *use* yet)
* callbacks passed to built-in functions correctly infer types of arrays
* === 0/0.0/false/''/[] does not force *mixed*
* added `openssl_pkey_get_public()`
* tuples can now be used inside forks
* more heuristics of classes detection to write less PHPDoc


## 2018-09-13

* tuples support
* `@kphp-template` functions support
* added `array_pad()`
* lint `global` keyword usage
* support `ClassName::class`
* read access by index `$a[$i]` doesn't force *$a* to be an array (it can be a string) 
* better stack traces when typing restrictions unmet
* better error messages in various places
* instances can now be used inside forks


## 2018-07-23

* check for uninitized variables
* added `openssl_x509_checkpurpose()`, `openssl_x509_parse()`
* lots of runtime functions behaves like PHP 5.6, not PHP "as happens"


## 2018-05-28

* PHPDoc of instance fields `/** @var */` is analyzed and checked
* `mkdir()` accepts `$recursive` argument
* `const` in global scope
* various bugfixes around instances and circular dependencies recompilation


## 2018-04-23

* classes with static functions, static fields, and constants
* static inheritance and late static binding
* classes with instance fields and methods (no OOP, just structures) and lots of limitations
* `@kphp-infer` annotation telling the compiler to analyze PHPDoc

**Classes first-version limitations**
* instances are not compatible with *mixed*
* access only declared fields, no dynamic fields creation
* *null* is unassignable to instances, but *false* is
* no magic methods
* no extends, implements, traits
* instances are not convertible to arrays
* you can't pass instances to built-in functions like *json_encode()* and others accepting *mixed*
* *$this* isn't captured by lambdas passed to built-in functions inside methods
* sometimes you should hint KPHP with PHPDoc (not all usage patterns recognized)
* and many minor others


## Earlier

Before 2018, the Changelog was not maintained.
