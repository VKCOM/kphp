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

For `ffi_scope<>`, you can use a pointer-type expression, builtin type name, struct name and
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

KPHP will give compile-time error if types are not compatible
or some requested type or function are not declared as a part of the FFI cdef source.

> Unlike in PHP, it's possible to use FFI::scope without opcache preloading.
> In order to be compatible with PHP, you need to provide a separate preloading
> script and run it appropriately.

When [\FFI::cdef](https://www.php.net/manual/ru/ffi.cdef.php) or [\FFI::load](https://www.php.net/manual/ru/ffi.load.php)
C sources don't contain a `FFI_SCOPE` directive, the scope name will be automatically generated,
so it will be almost impossible to spell that `ffi_scope` type properly.
For both PHP and KPHP it's preferable to use named scopes. 


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
