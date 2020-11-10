---
sort: 4
title: What exactly would be compiled?
---

# What PHP code exactly would be compiled?

You invoke `kphp file.php` — and it compiles everything reachable from *file.php*.


## KPHP parses only explicitly required files

Suppose you have the following files:
```php
// file1.php
define('SOME_CONST', 123);
require_once 'lib.php';
function f1() { lib(); }
f1();

// file2.php
require_once 'lib2.php';
function f2() { lib(); }
f2();

// lib.php
function lib() { echo "const = " . SOME_CONST; }
```

**If you compile file1.php — ok** 

KPHP sees that *lib.php* is required, parses it, sees that *f1()* is called => *lib()* is reachable, everything is ok.  
Even if *file2.php* is syntactically incorrect, there would be no error, as it is not reachable.

**If you compile file2.php — undefined constant**

KPHP requires *lib.php*, sees that *f2()* is called => *lib()* is reachable, and **receives a compilation error** because *SOME_CONST* is undefined. 
It is defined in *file1.php*, that file is not reachable while compiling *file2.php*.

```tip
You can easily use PHPUnit and other development tools — just don't make it explicitly reachable.
```


## KPHP compiles only explicitly called functions

Let's extend *lib.php*:
```php
// lib.php
function lib() { echo "const = " . SOME_CONST; }

function parseStringRaw(string $x) {
  some_unknown_function();  // calling unknown
}

function parseString() {
  parseStringRaw(42);       // type mismatch
} 
```

Surprisingly, **the compilation will succeed**. Because two new functions **are not called from anywhere** — and thus not analyzed. They are just parsed, but are not included in a call graph cause not reachable.

Once you call *parseString()* from any reachable code, you'll get compilation errors.

There is a special annotation `@kphp-required` above a function to force KPHP to start analyzing this function even if it is not explicitly called. Primarily it is used for functions that are passed to callbacks just as string names:
```php
/** @kphp-required */
function comparator($a, $b): int { /* ... */ }
usort($arr, 'comparator');
// @kphp-required is needed, as comparator() isn't explicitly called but should exist much later
``` 


## Class autoloading and Composer modules

In PHP, you can theoretically write any autoloading logic — where to find a class.  
In KPHP, your classes should be organized in the only way — as KPHP locates them.

<p class="pay-attention">
    <b>1. Classes in your project must follow psr-4 naming in the include dir</b>
</p>

```
// in PHP, you write
use \SomeNamespace\Inner; 
Inner\ClassName::someMethod();

// after resolving, KPHP needs to locate this class
\SomeNamespace\Inner\ClassName

// it is supposed to be in the file
{include_dir}/SomeNamespace/Inner/ClassName.php
```

Include paths are set with `--include-dir / -I` option, it can appear multiple times as a console argument.

Again, only explicily used classes would be located:
```php
function this_f_never_called_from_reachable_code() {
  new \Some\Unknown\ClassName;
}
```
If the function is not reachable from a compiled entrypoint, *ClassName* would not be tried to be found. It can use unsupported features, or be lexically incorrect, or not exist — doesn't matter.

<p class="pay-attention">
    <b>2. Classes from Composer modules are located like in PHP</b>
</p>

Suppose you have the following folder structure:
```
my_libs/
  ...
vendor/
  ...
index.php
composer.json
```

To activate Composer discovery, pass `--composer-root` [option](./compiler-cmd-options.md): KPHP will parse *composer.json* and traverse *vendor/* folder located nearby. A common case is to specify the current folder:
```bash
kphp --composer-root $(pwd) index.php
```

```note
This allows you to distribute and use KPHP-compatible modules via Composer.
```


## #ifndef KPHP

This construction "hides" code from KPHP, so that it is seen and executed **only in PHP**:

```
#ifndef KPHP

code here is invisible for KPHP but visible for PHP 
(sign '#' is just a line comment for PHP)

#endif
```

Why may you need it?
* to enable PHP runtime setup: autoload, include path, ...
* detailed error tracing and exception handler for development
* some development-only or unit-testing-only logic
* fix differences between PHP and KPHP runtime behavior

Let's say *file2.php* now looks like this:
```php
// file2.php
require_once 'lib2.php';
function f2() { lib(); }
#ifndef KPHP
f2();
#endif
```

Now, if you compile it, there will be no compilation error: *f2()* not called => *lib()* not reachable => undefined *SOME_CONST* usage is not reachable. 


## define('kphp')

A suggestion is to include the following snippet to your project:
```php
#ifndef KPHP
define('kphp', 0);
if (false)
#endif
define('kphp', 1);
```

Then you can use it like a regular costant:
```php
header(kphp ? 'Powered by KPHP' : 'Powered by PHP');
```

This also helps fix differences between PHP/KPHP if you encounter some.  

But still, avoid using these constructions as much as possible — because it can easily lead to unexpected behavior when your code works differently after compilation.


```tip
Due to reachability, you can organize your PHP project in a way, that part of it would be compiled, and part of it would still work on plain PHP.  
For example, file uploading can remain PHP-based, whereas API business logic works on KPHP.
```
