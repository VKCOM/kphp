---
sort: 4
title: vkext
---

# KPHP internals — stage 4: vkext

**vkext** KPHP stage exposes the network layer and TL/RPC as a PHP extension for development.

Technically, vkext is not a part of the KPHP compiler: it's a PHP extension. Without vkext, PHP code can still be compiled and run, but using TL would be unavailable in plain PHP. Other KPHP functions (apart from network-related) don't require vkext, they are just [polyfilled](../../kphp-language/php-extensions/php-polyfills.md).

vkext is written in C and C++. Its code is placed in the *vkext/* folder and, like any other Zend-related code, it is full of macros and is hard to read unless you have had some experience with Zend internals. 


## Some words about writing PHP extensions

The main problem is a lack of documentation about Zend API used in PHP 7. Functions and parameters naming is often unobvious. What are the main resources of PHP extensions?
* [phpinternalsbook.com](https://www.phpinternalsbook.com/), but many sections are just empty
* the book [about PHP 5 internals](https://books.google.ru/books?hl=ru&lr=&id=zMbGvK17_tYC&oi=fnd&pg=PP1&dq=extending+and+embedding+php&ots=0cfb6329mb&sig=0LCMOuKPU1tSW2tsQe3g8J88GnE&redir_esc=y#v=onepage&q=extending%20and%20embedding%20php&f=false) is urgent if PHP 7 changes are not significant; by the way, it's a worthwhile book, covering PHP lifecycle and other topics
* the blog by [nikic](https://nikic.github.io/) about PHP 7 core
* consult actual PHP sources [on GitHub](https://github.com/php/php-src)

While writing PHP extensions, you are constantly dealing with memory leaks: it's always unclear, whether arguments passed to Zend functions are copied or not.
* be very careful about refcounters; for instance, the function updating class field value `void zend_update_property(..., zval *value)` increments refcount; if it's unexpected (if you want just to move a value to a field), don't forget to call `zval_ptr_dtor(value)` to decrement it back
* use debug build of PHP, it shows leaks also
* use profiling tools, valgrind for example; when *vkext* is enabled in *php.ini*, it could be [launched as](https://www.phpinternalsbook.com/php7/memory_management/memory_debugging.html)
```bash
ZEND_DONT_UNLOAD_MODULES=1 USE_ZEND_ALLOC=0 valgrind --log-file=valgrind.out --leak-check=full --show-reachable=yes --track-origins=yes php main.php
```

```note
Writing PHP extensions is a separate area of knowledge, but it's very unlikely you'll need to patch *vkext* because polyfills are almost always enough to make KPHP built-ins available in plain PHP.
```
