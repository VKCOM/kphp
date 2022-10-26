---
sort: 7
noprevnext: true
title: What's new?
---

# What's new? (Versions history)


## [2022-10-06](https://github.com/VKCOM/kphp/milestone/26?closed=1)

* Modulite + Composer integration
* allow unpacking into positional arguments `f(...[1,2])` for `function f($x,$y)`
* reorder class fields at codegeneration considering C++ fields padding to reduce object size
* class `DateTime`
* added argument `$rest_index` to `getopt()`
* added `strspn()` and `strcspn()`
* optimize `switch` codegen if all cases are constant strings/ints
* compatible with leak sanitizer
* optimize `strtolower()` and `strtoupper()` to produce less allocations
* g++ is launched with `-O2` by default


## [2022-09-01](https://github.com/VKCOM/kphp/milestone/25?closed=1)

* FFI callbacks supported, Lua modules can now be embedded ([doc](../kphp-language/php-extensions/ffi.md))
* added `ignore_user_abort()` to prevent script terminations in distributed transactions
* constants with leading slash now work
* parse `array<int, string>` in phpdocs, type of key is just dropped off
* added `stripcslashes()`
* added argument `$scale` to `bcpow()` and `bcmod()`
* whitespace identation at heredoc/nowdoc
* class `ArrayIterator` (for `mixed[]` arrays only)
* `dirname()` works inside `require` when compile-time resolved
* support psr-0 autoload in *composer.json*


## [2022-08-12](https://github.com/VKCOM/kphp/milestone/24?closed=1)

* generic functions `@kphp-generic`: next gen template functions ([doc](../kphp-language/static-type-system/generic-functions.md))
* deprecate `@kphp-template`, `@kphp-param`, and `@kphp-return`
* support Modulite inside kphp (everything except composer) 
* http 103 headers, `send_http_103_early_hints()`
* added `array_is_list()`
* added `instance_deserialize_safe()` and `instance_serialize_safe()`


## [2022-07-13](https://github.com/VKCOM/kphp/milestone/23?closed=1)

* reduce binary size with debug symbols by embedding common template specializations at runtime compilation
* `JsonEncoder` and `@kphp-json` ([doc](../kphp-language/howto-by-kphp/json-encode-decode.md))
* FFI improvements: nested pointers, arrays, native strings
* added argument `$offset` to `preg_match_all()`
* `unpack()` returns `false` on error


## [2022-06-09](https://github.com/VKCOM/kphp/milestone/22?closed=1)

* shutdown functions are invoked even after script timeout
* embedded msgpack instead of an external dependency
* `pack()` and `unpack()` support `e` and `E` formats


## [2022-05-26](https://github.com/VKCOM/kphp/milestone/21?closed=1)

* some optimizations reducing binary size for vkcom
* `microtime(true)` returns `float` (support specializations for built-in functions)
* added `date_parse()` and `date_parse_from_format()`
* added option `--sigterm-wait-time`


## [2022-04-15](https://github.com/VKCOM/kphp/milestone/20?closed=1)

* better CPU utilization (NUMA, CPU affinity, multiple socket backlogs)
* added class `CompileTimeLocation` ([doc](../kphp-language/howto-by-kphp/compile-time-location.md))
* added `array_unset($arr, $key)`
* added `memory_get_allocations()` to be used in ktest
* `intval()` ingores spaces at string start
* on json encoding error, output a full path to that key
* compatible with clang++-11
* support FFI pointers ([doc](../kphp-language/php-extensions/ffi.md))


## [2022-02-18](https://github.com/VKCOM/kphp/milestone/19?closed=1)

* added `to_array_debug(object|tuple|shape)`
* handle phpdoc above lambdas
* better deal with phpdoc/typehints mixture on inheritance
* MySQL resumable support and basic PDO implementation
* callbacks passed to internal functions restricted to be resumable
* optimize phpdoc parsing
* added `str_starts_with()` and `str_ends_with()`


## [2022-01-20](https://github.com/VKCOM/kphp/milestone/18?closed=1)

* use lld instead of ld for partial linking
* support indexing in constants declaration
* fire compilation errors for invalid indexing typing
* some changes in precompiled headers for nocc integration
* added `array_merge_recursive()`


## [2021-12-22](https://github.com/VKCOM/kphp/milestone/17?closed=1)

* lambdas re-implemented completely, almost all relative issues resolved
* template functions re-implemented completely, almost all relative issues resolved
* a new syntax for `@kphp-template`, we're heading towards generics
* argument `$flags` in `array_unique()`


## [2021-12-09](https://github.com/VKCOM/kphp/milestone/16?closed=1)

* output some metrics to statshouse instead of grafana
* `$exception->getFile()` returns relative paths for stable codegen in TeamCity
* `Exception` and other built-in classes became copyable into shared memory
* KPHP can now be compiled for Apple M1


## [2021-11-11](https://github.com/VKCOM/kphp/milestone/15?closed=1)

* fix a race condition in `KphpJobWorkerMemoryPiece`
* initial FFI support ([doc](../kphp-language/php-extensions/ffi.md))


## [2021-10-15](https://github.com/VKCOM/kphp/milestone/14?closed=1)

* dropped Jessie support, C++ 17 is available for development
* full UTF-8 and complex emoji support
* increased http server limits
* existing bcmath functions work like PHP in corner cases


## [2021-09-16](https://github.com/VKCOM/kphp/milestone/13?closed=1)

* added `set_migration_php8_warning($bitmask)` that will trigger warnings on PHP 7/8 behavior mismatch
* some fixes around sigsegv in kubernetes restart


## [2021-08-26](https://github.com/VKCOM/kphp/milestone/12?closed=1)

* links to equal instances remain after copying to shared memory
* supported copying hierarchical classes to shared memory
* smart casts for ternary expressions


## [2021-07-30](https://github.com/VKCOM/kphp/milestone/11?closed=1)

* spread operator `...$array`
* numeric literal separator: `107_925_284.88`
* all tests correctly work with PHP 7.4


## [2021-07-15](https://github.com/VKCOM/kphp/milestone/10?closed=1)

* job workers released: separate processes working in parallel with http workers ([doc](../kphp-language/best-practices/parallelism-job-workers.md))


## [2021-06-18](https://github.com/VKCOM/kphp/milestone/9?closed=1)

* use shared memory instead of pipes for stats collecting from workers into master
* some compile-time optimizations
* added `str_ireplace()`
* added constants `M_E`, `M_PI_2`, and similar
* support for token `INF`


## [2021-06-02](https://github.com/VKCOM/kphp/milestone/8?closed=1)

* fix memory leak in curl library for long-running processes
* support `__toString()` magic method
* beta version of job workers
* compatible with g++-11
* generate pch header for clang, same as gch for g++


## [2021-04-29](https://github.com/VKCOM/kphp/milestone/7?closed=1)

* lots of codegeneration optimizations, 20% speedup for vkcom
* colored functions `@kphp-color` ([doc](../kphp-language/howto-by-kphp/colored-functions.md))
* some fixes around lambdas, interfaces and traits
* consider type hints compatibility on inheritance, the same way as PHP does


## [2021-04-02](https://github.com/VKCOM/kphp/milestone/6?closed=1)

* reorganized runtime code to achieve more accurate cpp backtraces in Sentry
* `strtotime()` now behaves exactly like in PHP due to integrated timelib
* support zstd functions
* `@kphp-warn-performance` above classes ([doc](../kphp-language/best-practices/performance-inspections.md))


## [2021-03-13](https://github.com/VKCOM/kphp/milestone/5?closed=1)

* support field type hints in classes
* `declare(strict_types=1)` prevents auto casts passing arguments to built-in functions
* a bit more exact typing for built-in preg functions
* `strip_tags()` supports self-closing tags like <br>
* `hash()` and `hash_hmac()` support sha224, sha384, sha512
* added `hash_hmac_algos()`
* pseudo-vectors encoded as regular vectors in msgpack serialization


## [2021-02-17](https://github.com/VKCOM/kphp/milestone/4?closed=1)

* significant reduced memory consumption for large codebases
* `void` functions are forbidden to fork, `future<void|null>` is invalid
* introduced `set_wait_all_forks_on_finish()`
* aliases for `array_reserve()` (for vector, map int keys, map string keys)
* added `acosh()`, `asin()`, `asinh()`, `cosh()`, `sinh()`, `rad2deg()`
* support dereferencing like `"asdf"[$i]` and `(function(){...})()`


## [2021-01-28](https://github.com/VKCOM/kphp/milestone/3?closed=1)

* pretty errors and call stack on type mismatch
* prevent chain reaction polluting types on type mismatch when restricted by phpdoc
* exception inheritance
* all standard exception classes implemented, including `Error` and `Throwable`
* `@kphp-throws` and `@kphp-should-not-throw` annotations
* significant codegeneration speedup for large codebases
* `to_array_debug()` accepts the second argument `bool $with_classnames=false`
* added `hrtime()`
* `parse_url()` behaves like PHP 7.4


## [2020-12-23](https://github.com/VKCOM/kphp/milestone/2?closed=1)

* `@return` now infers the type, not just checks it
* union type hints and `mixed` type hint are now valid not only in phpdoc
* `@kphp-serialized-float32` annotation above fields for msgpack (4 bytes, not 8)
* compatible with MacOS


## [2020-12-11](https://github.com/VKCOM/kphp/milestone/1?closed=1)

* json logs are also written from signal handlers and uncaught exceptions
* `@kphp-analyze-performance` and `@kphp-warn-performance` ([doc](../kphp-language/best-practices/performance-inspections.md))
* finalization of 64 bit support
* correctly handle calls with a leading slash `\funcName()`
* correctly handle `"${var}"` interpolation (previously, only `"{$var}"` worked)


## 2020-11-19

* TL `Long` is now `int` at codegen, not `mixed`
* use critical sections around OpenSSL runtime


## 2020-10-27

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

* `to_array_debug()`
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
