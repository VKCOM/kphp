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
feel free to [file an issue](https://github.com/VKCOM/kphp/issues/new).

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

### Auto conversions

If PHP would apply an auto conversion, KPHP would do it too.

There are 2 kinds of auto conversions:

* `php2c` -- when a PHP value is being passed or assigned to a C value
* `c2php` -- when a C value is being read (so it becomes a PHP value)

Many `php2c` and `c2php` conversions are symmetrical, but there are a few special rules we should keep in mind.

When a `php2c` can happen:

* Passing C function argument
* Assigning to a C struct/union field
* Assigning to a pseudo `cdata` property of C scalar types (like `int8_t`)

| PHP type | C type | Note |
|---|---|---|
| `int` | `int8_t`-`int64_t`, `int`, `uint`, etc. | A trivial cast (may truncate). |
| `float` | `float`, `double` | A trivial cast. |
| `bool` | `bool` | No-op. |
| `string(1)` | `char` | Run time error if string length is not 1. |
| `string` (*) | `const char*` | Doesn't make a copy. |
| `ffi_cdata<$scope, T>` | `T` | Alloc-free. |
| `ffi_cdata<$scope, T*>` | `T*` | Alloc-free. |

> (*) Only for function arguments, but not struct/union fields write.

When a `c2php` can happen:

* Assigning a non-void C function result
* Reading C struct/union field
* Reading C scalar pseudo `cdata` property
* Reading FFI Scope property (C enums, extern variables)

| C type | PHP type | Note |
|---|---|---|
| `int8_t`-`int64_t`, `int`, `uint`, etc. | `int` | A trivial cast. |
| `float`, `double` | `float` | A trivial cast. |
| `bool` | `bool` | No-op. |
| `char` | `string(1)` | Allocates a string of 1 char. |
| `const char*` (*) | `string` | Allocates a string, copies data from `const char*` |
| `T` | `ffi_cdata<$scope, T>` | Allocates a `ffi_cdata` object. |
| `T*` | `ffi_cdata<$scope, T*>` | Alloc-free. |

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
