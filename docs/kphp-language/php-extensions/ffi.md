---
sort: 4
title: FFI
---

# FFI (foreign function interface)

KPHP implements PHP-compatible [FFI](https://www.php.net/manual/en/book.ffi.php) APIs.

A valid KPHP code that uses FFI will behave identically when executed with PHP interpreter.

This means that you can implement some C library wrappers in PHP via FFI and use it
in both KPHP and PHP modes. These libraries can be bundled as composer packages.

If you have any questions on FFI, you should probably refer to the PHP documentation.
This page contains some minimal information and KPHP-specific notes.

In case you're missing some feature that is not yet implemented in KPHP FFI system,
feel free to [file an issue]({{site.url_github_issues}}/new).

## What is FFI?

A [foreign function interface](https://en.wikipedia.org/wiki/Foreign_function_interface) is useful
to extend KPHP with functionality that is already implemented in C.

It's particularly helpful for KPHP as you can implement unsupported PHP extensions yourself
by writing a code that uses the underlying C library that is used by the PHP internal implementation.

For instance, KPHP does not come with [GD](https://www.php.net/manual/ru/book.image.php) module.
If you look at the PHP implementation, you'll notice that it implements most functions using
[libgd](https://libgd.github.io/) C library. This is where FFI comes in handy: if you create
a FFI class that wraps `libgd`, you'll have an ability yo use GD in both PHP and KPHP.

Another use case is using something completely wild and unusual, like [SDL](https://wiki.libsdl.org)
library for game development. There is an example of [KPHP game](https://github.com/quasilyte/kphp-game)
that is created thanks to the FFI and SDL.

FFI is also a preferred way to extend PHP. It's less likely to break as PHP versions go up.
It's also going to become faster for PHP when JIT will recognize these calls.
As for KPHP, our FFI implementation is quite efficient and could become even more optimized in the future.
There are a few gotchas, but you generally should not notice any performance problems with it.


## KPHP-specific features, extensions


### Type hints

As with anything else, KPHP wants to have more types information to build your code.

All FFI-related types should be annotated with special annotations:

* `ffi_cdata<scope_name, type_expr>` for [\FFI\CData](https://www.php.net/manual/ru/class.ffi-cdata.php)
* `ffi_scope<scope_name>` for [\FFI](https://www.php.net/manual/ru/class.ffi.php) class instances

For `ffi_cdata<>`, you can use a pointer-type expression, builtin type name, struct name and
any other supported C type expression. For instance, `ffi_cdata<foo, union Event**>` is a valid
`ffi_cdata` declaration.

This example should make it obvious when you need to use either of them:

```php
<?php

/** @param ffi_scope<example> */
function f($lib) {
  $foo = $lib->new('struct Foo');
  $foo->value = 1204;
  g($foo);
  g_ptr(\FFI::addr($foo));
}

/** @param ffi_cdata<example, struct Foo> */
function g($foo) { var_dump($foo->value); }

/** @param ffi_cdata<example, struct Foo*> */
function g_ptr($foo) { var_dump($foo->value); }

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  struct Foo { int value; };
');

f($cdef);
```

If you need to describe a builtin C type that is not bound to any particular FFI scope,
use default FFI scope name "C":

```php
/**
 * Since void* is a global type, we can use "C" scope name to avoid any dependency
 * on a custom FFI scope.
 *
 * @param ffi_cdata<C, void*> $ptr
 */
function as_uint8_ptr($ptr) {
  FFI::cast('uint8_t*', $ptr);
}
```

KPHP will give compile-time error if types are not compatible
or some requested type or function are not declared as a part of the FFI cdef source.

> Unlike in PHP, it's possible to use FFI::scope without opcache preloading.
> In order to be compatible with PHP, you need to provide a separate preloading
> script and run it appropriately.

When [\FFI::cdef](https://www.php.net/manual/ru/ffi.cdef.php) or [\FFI::load](https://www.php.net/manual/ru/ffi.load.php)
C sources don't contain a `FFI_SCOPE` directive, the scope name will be automatically generated,
so it will be almost impossible to spell that `ffi_scope` type properly.
For both PHP and KPHP it's preferable to use named scopes. 

### Array types

It is possible to allocate both fixed size and dynamic sized arrays in FFI:

```php
$fixed_arr = \FFI::new('int32_t[8]');

$size = 15;
$dynamic_arr = \FFI::new("int32_t[$size]");
$dynamic_arr2 = \FFI::new('int32_t[' . $size . ']');
```

To specify that type in a type hint, use this notation:

```php
class Example {
  /** @var ffi_cdata<C, int32_t[]> */
  public $ffi_arr;

  /**
   * A PHP array of FFI arrays
   * @var ffi_cdata<C, int32_t[]>[]
   */
  public $array_of_ffi_arrays;
}
```

Note that you can pass fixed size arrays as dynamic size array types.

It is possible to cast a `T[]` array to a `T*` pointer.
Unlike the pointers acquired from `FFI::cast()`, arrays retain data ownership.
So while it is usually safe to pass `T[]` as `T*` as a function argument,
it is better to return these values as `T[]` to avoid dangling pointers.

Some operations require kphp polyfills due to the different syntax.

```php
// get element of an array (or dereference a pointer at given offset)
// identical to $arr[$index];
$result = ffi_array_get($arr, $index);

// set element of an array (or write at the pointer at given offset)
// identical to $arr[$index] = $value;
ffi_array_set($arr, $index, $value);

// like FFI::memcpy, but the source argument is a string, not FFI\CData
// identical to FFI::memcpy($cdata, $php_string, $size);
ffi_memcpy_string($cdata, $php_string, $size);

// like FFI::cast, but the source argument is int, not FFI\CData
// identical to FFI::cast('void*', $addr);
// $addr should be a value obtained by ffi_cast_ptr2addr
ffi_cast_addr2ptr($addr)

// like FFI::cast, but the destination type is uintptr_t-like scalar
// identical to FFI::cast('uintptr_t', $ptr)
// $ptr should be a CData holding void* pointer
ffi_cast_ptr2addr($ptr)
```

### Auto conversions

If PHP applies an auto conversion, KPHP would do it too.

There are 2 kinds of auto conversions:

* `php2c` -- when a PHP value is being passed or assigned to a C value
* `c2php` -- when a C value is being read (so it becomes a PHP value)

Many `php2c` and `c2php` conversions are symmetrical, but there are a few special rules we should keep in mind.

When a `php2c` can happen:

* Passing C function argument
* Assigning to a C struct/union field
* Assigning to a pseudo `cdata` property of C scalar types (like `int8_t`)

| PHP type                | C type                                  | Note                                      |
|-------------------------|-----------------------------------------|-------------------------------------------|
| `int`                   | `int8_t`-`int64_t`, `int`, `uint`, etc. | A trivial cast (may truncate).            |
| `float`                 | `float`, `double`                       | A trivial cast.                           |
| `bool`                  | `bool`                                  | No-op.                                    |
| `string(1)`             | `char`                                  | Run time error if string length is not 1. |
| `string` (*)            | `const char*`                           | Doesn't make a copy.                      |
| `ffi_cdata<$scope, T>`  | `T`                                     | Alloc-free.                               |
| `ffi_cdata<$scope, T*>` | `T*`                                    | Alloc-free.                               |

> (*) Only for function arguments, but not struct/union fields write.

For int, float and bool it's also possible to use a nullable type.
`?int` can be used as any compatible C numeric type (int8_t, etc.) with null being interpreted as 0.
For bool types, false is used for null values.

When a `c2php` can happen:

* Assigning a non-void C function result
* Reading C struct/union field
* Reading C scalar pseudo `cdata` property
* Reading FFI Scope property (C enums, extern variables)

| C type                                  | PHP type                | Note                                               |
|-----------------------------------------|-------------------------|----------------------------------------------------|
| `int8_t`-`int64_t`, `int`, `uint`, etc. | `int`                   | A trivial cast.                                    |
| `float`, `double`                       | `float`                 | A trivial cast.                                    |
| `bool`                                  | `bool`                  | No-op.                                             |
| `char`                                  | `string(1)`             | Allocates a string of 1 char.                      |
| `const char*` (*)                       | `string`                | Allocates a string, copies data from `const char*` |
| `T`                                     | `ffi_cdata<$scope, T>`  | Allocates a `ffi_cdata` object.                    |
| `T*`                                    | `ffi_cdata<$scope, T*>` | Alloc-free.                                        |

> (*) Only for function results, but not struct/union fields reads.

Keep in mind: KPHP will try to behave identically to how PHP would. This means that marking a function return as `const char*` if it needs a `free()` will cause a **memory leak** in both PHP and KPHP.

How that memory leak would happen:

1. Some C code allocates memory.
2. We return it as `const char*`.
3. KPHP/PHP copies the underlying data (interpreting it as a C string, null-terminated).
4. Then we `free()` the copied data, but not the original pointer!

There is no way to `free()` the initial pointer in this case, so the memory leak is imminent.

`const char*` is safe in several scenarios, some examples:

* The data pointed by `const char*` is static (not allocated in heap)
* The data pointed by `const char*` is managed by a C code itself (reused or whatever)

> Warning: you have to understand how PHP FFI works in order to use it correctly. C headers should also be carefully ported for PHP FFI as it does do some auto conversion magic depending on the types. You want them to be applied where suitable and avoid potential issues like the one mentioned above.

Auto conversions affect the inferred types too. Here is an example:

```php
<?php

$cdef = FFI::cdef('
    const char *getenv(const char *name);
    int abs(int x);
    struct Foo {
        const char *str_field;
    };
');

// $home type is `PHP string`, since `const char*` has
// an auto conversion defined for a function result;
// a function argument is auto converted from `PHP string`
// to a `const char*` (no allocations are made)
$home = $cdef->getenv('HOME');

$foo = $cdef->new('struct Foo');

// $str_field type is `ffi_cdata<..., const char*>`, since
// `const char*` can't be converted in this context
$str_field = $foo->str_field;

// This is illegal, no auto conversion is available in this
// context, a `PHP string` can be converted to a `const char*`
// only in the context of function arguments passing
$foo->str_field = 'foo'; // ERROR

// On most systems C `int` type is `int32_t`; KPHP ints are 64-bit wide.
// When we pass a `PHP int` type, we're applying an auto conversion (php2c)
// that looks like static_cast<int32_t>($x).
// When we return a C `int` value, it's converted to a `PHP int` type,
// using an auto conversion again (c2php), this time it's static_cast<int64_t>($x).
$v = $cdef->abs(10);
```

### Function pointer types

It's possible to pass KPHP functions as C function pointer (callback function).

To do this, several limitation should be kept in mind:

* For methods, only static methods are allowed
* For lambdas, no capturing is allowed (`use` is not supported)
* FFI callbacks can't throw exceptions (it would be a compile-time error in KPHP)

There is a high chance that you'll need to have an FFI scope object inside that lambda to make it useful.
In order to access it from the lambda without captures, use global variables or static class fields.

Some APIs provide a common `void*` user data parameter. You pass that argument to the function along
with the callback, and it will be passed to you callback along other arguments.

```php
// $user_data is Context*
$lib->some_func(FFI::cast('void*', $user_data), function ($ud) {
  $data = MyLib::$scope->cast('struct Context*', $ud);
  // use $data as Context*
});
```

The callbacks are type checked, so you can't use a function with incompatible types.

Special rules are used when mapping lambda params and result types. For auto-convertible types
like string, int and float, unboxed types are used. For other cdata types, boxed forms are used.

| C callback signature           | PHP lambda signature                                             |
|--------------------------------|------------------------------------------------------------------|
| `int16_t (*) (uint32_t, bool)` | `callable (int, bool): int`                                      |
| `void* (*) (struct Vector2f*)` | `callable (ffi_cdata<lib, struct Vector*>): ffi_cdata<C, void*>` |
| `void (*) (const char*)`       | `callable (string): void`                                        |
| `char (*) ()`                  | `callable (): string`                                            |

How FFI lambdas work:

* A special wrapper function is generated, it accepts params identical to C callback signature types
* The wrapper forwards all C arguments to KPHP lambda using c2php
* Then it returns the result of the KPHP lambda via php2c

So, C side can call KPHP lambda using C types and get C types returned from it.

All foreign code is executed inside **critical section**. That critical section is suspended until KPHP lambda returns.
After that, critical section is entered again. This means that you can call other FFI functions while running  KPHP lambda.

When writing the function pointer type in FFI C headers, you have two options:

1. Create a typedef

```c
typedef RetType (*FuncTypeName) (ParamType1, ..., ParamTypeN);

void use_callback(FuncTypeName f);
```

2. Use function pointer inline syntax

```c
void use_callback(RetType (*f) (ParamType1, ..., ParamTypeN));
```

KPHP callbacks should be used carefully. If you allocate a managed FFI object inside lambda and return
that object to C, it will be deallocated by the time KPHP callback returns. This means C will get
some potentially invalid memory without even knowing it.

Therefore, sometimes it's better to allocate unmanaged memory with `FFI::new()` and then call `FFI::free()` manually.
The exact advice would depend on the library you're trying to use via FFI and its API.

It's safe to return scalar and compound value types (`T` instead of `T*`) as they will be copied properly.
(If object of type `T` contains dynamically allocated memory, it's another topic.)

To pass a null function pointer, simply use `null` literal:

```php
// the C side will get null pointer as function pointer argument
$c_lib->c_func(null);
```


### Passing C function as C callback

Let's suppose you want to pass some C library function as C function callback.

The straightforward approach may not always work:

```php
// may fail during the run time
Foo::$lib->some_func(Foo::$lib->other_func);
```

As a workaround, you can use extra lambda wrapping:

```php
Foo::$lib->some_func(function ($x) {
  return Foo::$lib->other_func($x);
});
```

This approach should always work.


### Non-owned (unmanaged) memory

When `FFI::new()` is called with two parameters and second parameter being `false`, the allocated object
will not be freed when its reference count goes to 0.

```php
// allocate 10 bytes that will not be automatically de-allocated;
// prefer uint8_t[...] to char[...] if you want a raw memory
$mem = FFI::new('uint8_t[10]', false);
```

The unmanaged (non-owned) memory is allocated using the script allocator.
This means that even if you forget to `free()` this memory, it won't leak at the request boundary.
It is still recommended to `FFI::free()` when possible to avoid out-of-memory errors during the request.

It's important that you call `FFI::free()` on the properly-typed object.
If memory was allocated as `uint64_t` (8 bytes), the pointer passed to `FFI::free()` should have that type.

```php
// suppose that $ptr is typed as `void*`, but originally we allocated 10 bytes;
// we need to cast that back to make sure free() works correctly;
$mem = FFI::cast('uint8_t[10]', $ptr);
FFI::free($mem);
```

If you're allocating a chunk of memory with something like a char array, then make sure to
free that chunk using the array with the same size that you used during the allocation.

Most C memory management rules apply here. Avoid calling `FFI::free()` on the same memory more than once and so on.

Please avoid the patterns illustrated below:

```php
// BAD: calling free() on FFI::addr() result
$obj = FFI::new('uint64_t', false);
FFI::free(FFI::addr($obj));

// GOOD: calling free() on the actual object you allocated
$obj = FFI::new('uint64_t', false);
FFI::free($obj);
```

```php
// BAD: casting an array to void* without doing FFI::addr()
$obj = FFI::new("int[$size]", false);
$ptr = FFI::cast('void*', $obj);
FFI::free($ptr);

// GOOD: using FFI::addr() on array
$obj = FFI::new("int[$size]", false);
$ptr = FFI::cast('void*', FFI::addr($obj));
FFI::free($ptr);
```

```php
// BAD: free() is called on an array of wrong size
$arr = FFI::new('int[10]', false);
$arr_ptr = FFI::cast('void*', FFI::addr($arr));
$arr2 = FFI::cast('int[5]', $arr_ptr);
FFI::free($arr2);

// GOOD: free() is called on an array of correct size
$arr = FFI::new('int[10]', false);
$arr_ptr = FFI::cast('void*', FFI::addr($arr));
$arr2 = FFI::cast('int[10]', $arr_ptr);
FFI::free($arr2);
```

The memory may not be reclaimed immediately after `FFI::free()` is called, but using any object pointing to
the underlying C data after freeing the memory can be considered to be undefined behavior.

Also note that globally-declared (including class static members) memory may follow different patterns.
If you need to free such global memory, assign `null` right after the `FFI::free()` call.

```php
// Combined together, this pattern gives you better chances that the
// memory will be freed earlier
FFI::free(SomeClass::$data);
SomeClass::$data = null;
```


### Static libraries

KPHP supports static libraries in addition to dynamic libraries.

The main benefit is that your compiled binary won't depend on some shared dependency
at the run time. You can embed that library right into the executable and distribute it
to your users and/or servers.

In order to use a static library, use `FFI_STATIC_LIB` instead of `FFI_LIB` directive:

```diff
-#define FFI_LIB "somelib.so"
+#define FFI_STATIC_LIB "/path/to/static/somelib.a"
#define FFI_SCOPE "example"
```

If KPHP can't locate that static library during the compilation (link time, really), it'll give an error.

The compiled executable (CLI or server) won't need that library to run.


### FFI loading and C parsing

PHP parses `cdef` and `load` C sources during the run time.
KPHP does this during the compilation, no C parsing is needed during the run time.
Although if you want to have a performant app runnable in both PHP and KPHP,
use PHP opcache preload to avoid redundant C parsing.

Dynamic FFI libraries are loaded at the moment `cdef` or `load` are executed.
Just like in PHP. It's not a good idea to use (non-static) FFI symbols prior
to the library being loaded.


## Useful tricks and hints

### Handling string results that are not null-terminated

Imagine that some C function result is declared as `const char*`. KPHP would try to create a `string` value, assuming that `const char*` contains a C-string value that is null-terminated.

If that's not the case and `const char*` is not null-terminated, you may need to declare that return type as something that is not `const char*`; a type like `const uint8_t*` should do the trick. While still compatible on the calling convention level, this allows us to disable the auto-conversion.

After acquiring a `const uint8_t*` pointer that holds some raw data, you can construct a string yourself if you know its real length by using `\FFI::string()` function.

```php
$cdef = FFI::cdef('
    const uint8_t *get_data(int size);
');

$size = 10;
$raw_bytes = $cdef->get_data($size);
$s = FFI::string($raw_bytes, $size);
```

Keep in mind that FFI/KPHP don't know anything about the data encoding. You may need to convert the raw bytes to some other valid state before using `\FFI::string()`. KPHP strings can contain binary data, but some string functions may not be binary-safe.

### Cross-library types

When you have more than one FFI scope, you may need to use the same type between them.

Let's try running this code:

```php
<?php

$cdef = FFI::cdef('
    struct Foo { int x; };
');
$cdef2 = FFI::cdef('
    typedef struct Foo Foo;
    struct Bar {
        struct Foo *fooptr;
    };
');

$foo = $cdef->new('struct Foo');
$bar = $cdef2->new('struct Bar');
$bar->fooptr = FFI::addr($foo);
```

PHP will give this error:

```
Incompatible types 'struct Foo*' and 'struct Foo*'
```

You may be confused by this message, but what it tries to say that `Foo` from one FFI scope is not compatible with `Foo` from another one. KPHP behaves the same.

To work around that, you have 2 approaches:

1. Use type erasure. Change `T*` to `void*` and handle that type as opaque pointer.
2. Use `FFI::cast()` method

The `(2)` is preferable. Here is a simple fix for the code above:

```diff
- $bar = $cdef2->new('struct Bar');
+ $bar->fooptr = $cdef2->cast('struct Foo*', FFI::addr($foo));
```

### FFI performance tips

For small, getter-like or pure math functions you may want to add a special comment inside the cdef string.

```c
// @kphp-ffi-signalsafe
double sin(double);
```

Note that you have to use a single-line comment for that, right before the function declaration.

```c
// @kphp-ffi-signalsafe
// this is a bad example
double sin(double);

// this is a good example
// @kphp-ffi-signalsafe
double sin(double);
```

With this comment, KPHP compiler will not emit a critical section for the specified function.

As the name implies, the function being called should be signal-safe.
Another way to put it: that function should not leak anything if it will be interrupted and never continued.

There are additional restrictions: a function marked with `@kphp-ffi-signalsafe` can't have
a function pointer argument.

This will affect only KPHP FFI performance as PHP will ignore that comment.


### Using languages other than C

If language `X` has a compiler that can produce a C-compatible libraries, you can call functions implemented in these languages via KPHP FFI.

It can be dangerous to use languages with heavy runtimes, especially if the runtime has garbage collector or is multi-threaded by default (even if you don't spawn any threads directly).

An example of such language is Rust. In this scenario, you'll have 2-layer FFI:

1. KPHP talking to C (our own FFI)
2. Rust talking to C (Rust FFI)

This means that it's important to learn both layers: how data is converted, how data ownership is managed, etc.

Another good case for FFI are languages that are easy to embed. For example, it should be relatively easy to write a Lua FFI library, so your KPHP code can evaluate arbitrary Lua scripts.

### Going beyond the FFI limits

KPHP FFI may not have some features implemented at the moment you'll need it.

Until we have a 100% production-ready support for FFI, there is a method to do everything you want right now. It's also PHP-KPHP compatible.

Let's suppose that the library you want to use via FFI has some complicated API that you can't express in the current version of KPHP FFI.

Instead of trying to use that library directly, you can create your own small C library that will call these functions for you. This library would be a glue (or a "bridge") to your FFI.

```
        FFI, KPHP->C             normal call, C->C
[ KPHP ]     --->    [ Bridge lib ]    --->    [ Target lib ]
```

This is still more convenient even for PHP as you won't need to do all the complicated steps required to build a "native" C PHP extension.


## Articles about using FFI in KPHP

Here are some articles on Habr (in Russian) about integrating external libraries using FFI:

* [Используем SQLite в KPHP и PHP через FFI](https://habr.com/ru/post/653677/)
* [Встраиваем Lua в PHP через FFI](https://habr.com/ru/company/vk/blog/681400/)
