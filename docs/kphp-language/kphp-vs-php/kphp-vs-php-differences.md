---
sort: 1
---

# KPHP vs PHP differences

```note
Historically, KPHP was developed at VK.com. VK was written in PHP and became very popular.  
PHP turned out to be a bottleneck for us, but we had no chances to switch to another language.   
We invented KPHP. It brings lots of limitations, but when satisfied, PHP code runs much faster.  
```


## KPHP is a compiler, PHP is an interpreter

First of all, KPHP is a **compiler**. It analyzes your whole project and compiles it to a single Linux binary.

More detailed, KPHP translates PHP sources to C++ equivalent and then compiles/links the resulting C++ code. 
Limitations of compilability take their roots from C++ — they are similar to all compiled languages.  

**In PHP**, if you made an error in your code — you'll see it only when the execution point reaches that line.  
**In KPHP**, you are unable to build your site until you fix all errors.

**In PHP**, symbols are resolved while executing. If you forget *require_once*, PHP will fail. You can *define()* in the middle of the script. Classes are dynamically autoloaded.   
**In KPHP**, all symbols are resolved at compile-time. You can call any existing function from anywhere — it just exists. All constants are inlined, they don't appear dynamically. A full list of classes is known in advance.

```tip
Typical development looks this way:  
1) you develop and test your code with PHP — it's very handy to use PHP as a development tool  
2) you compile your code with KPHP and deploy to production — it works identically, but faster
```  


## KPHP doesn't support features, that can't be compiled

```php
$func_name = "action_handler_" . ($input['action'] ?: 'default');
call_user_func($func_name, $_GET);
```
You can't call a function by name in any compiled programming language. But you can do this in PHP — and in PHP it's a very common scenario for routing/factories/etc. It won't work in KPHP.  
As a consequence, you can't call methods by name, you can't access variables by name, and so on. 

In brief, KPHP doesn't support fundamentally:
* accessing by a dynamic name
* eval
* declaring functions / classes / consts / etc. dynamically
* reflection
* mocks
* *reset()*, *current()* and other functions with internal array pointers
* interop with PHP extensions

As a consequence, KPHP won't compile standard frameworks / ORMs / PHPUnit.  


## KPHP doesn't support features, that break type system

```php
f(42);
f("string");
f(new User);
array(1, new User, function(){});
```

**In PHP**, you can mix types arbitrary and handle them at runtime. You pass numbers / arrays / objects to the same function. You create hashmaps of anything — and it just works if you access it by correct indexes.  
**In KPHP**, you should always think about types — like in any other compiled language. If you mix types incorrectly, it's a compilation error. You'll need to rewrite lots of your PHP code to satisfy the type system.

We'll discuss the type system [later](../static-type-system/kphp-type-system.md). The main point for now: restrictions are the reason for performance.

```tip
Consider the ["Various howto"](../howto-by-kphp) chapter for some bits of advice, how to deal with these limitations. 
```


## KPHP doesn't support features, that VK.com never had a need for

KPHP was developed at VK.com, and VK.com doesn't use standard databases like Postgres, Redis, and others. All storages and protocols are also self-written (but not open-sourced yet).  
Besides, not all PHP standard library is covered. For example, in VK.com mail sending and working with images is done with external engines, so KPHP has never needed *imagecreate()* and related stuff.

As a result, KPHP has no built-in support of many "real-world" needs:
* interop with Postgres / Redis / Tarantool / other well-known storages and DBs is missed at all
* `$_SESSION` superglobal
* mail sending is limited
* OpenSSL has some unsupported areas
* curl has some unsupported areas
* file access has a lot of unsupported areas
* working with images is not supported
* file uploading is not supported 
* XML/DOM parsing is not supported
* SPL classes are not implemented
* fastcgi_finish_request is not implemented and comes across current architecture
* `<?= ... ?>` short tags
* only *cp1251* and *UTF-8* encodings are supported

This can technically be done — and probably would be done in the future — but making the first public release, we realize, that it is probably the most crucial aspect of disappointment.

```tip
You can organize your project in a way, that part of it would be compiled, and part of it would work on PHP.  
For example, file uploading can remain PHP-based, whereas API business logic works on KPHP.
``` 


## Some details in PHP syntax are just not implemented

KPHP has about PHP 7.2 language level support.

Some parts of PHP syntax can be technically supported, but were not implemented — either they are hard to implement, or there wasn't enough time, or they are considered to be a bad style.

For now, KPHP has the absence of:
* nested *list()* assignments  
* array and string dereferencing
* generators
* group use declarations
* anonymous classes
* negative string offsets
* finally
* having a common interface more than once in parents chain
* insteadof and traits renaming
* func_get_args
* references are only partially supported: only foreach by reference and reference arguments are allowed

Most of them are not hard to implement, they were not just done yet.

<p style="color: grey">
    <i>(Yes, backend developers at VK.com don't use these features, but none of them are major or a priority to be added)</i>
</p>

```danger
As a result, KPHP would probably not compile any existing *not-too-primitive* library. 
All solutions for VK.com were written from scratch.  
```
```tip
How to overcome the current situation? 
* extend KPHP abilities 
* write [analogs](../php-extensions/php-snippets.md) to existing libraries, that would be compilable; probably, with the help of the community
```


## KPHP has some known differences in behavior against PHP

The killer feature of KPHP is that after compilation your code acts the same as in PHP.  
This is 99.9% true. In some strange or very rare cases, KPHP can evaluate differently.

KPHP may differ (and may not) in the following scenarios:
* strange math operations with invalid argument types; for example, if you subtract a number from an array, or if you calculate the tangent of a literal string
* standard library functions invoked with invalid arguments — like calling *substr()* with non-numeric limits
* some standard PHP functions work differently depending on PHP version; KPHP works as of luck
* bitwise operators assume integer operands (in PHP, they sometimes work for non-numeric strings)
* integer overflow behaves like native 64-bit (in PHP, an integer may be converted to floating-point)
* getting an unexisting element from a typed array [may return not null](../static-type-system/typed-arrays.md); typically, it's not a problem

It's unlikely to face misbehavior affecting production — but still, test a compiled site before deploying.   


## KPHP has features, that don't exist in PHP<br>             — the reason why KPHP is essential

Most of them are dedicated to compile-time checks and runtime performance.  

* strict typing, [type inferring](../static-type-system/type-inferring.md), [type checks](../static-type-system/phpdoc-to-declare-types.md) — no *ZVAL*, much faster
* async programming: [coroutines](../best-practices/async-programming-forks.md)
* shared memory across requests with special PHP API [to use it](../best-practices/shared-memory.md)
* compiler checks: constantness, immutability, checked exceptions, and others — via [annotations](./phpdoc-annotations.md)
* embedded web server, process orchestration, graceful restart [and more](../../kphp-server/kphp-as-backend/web-server.md)
* compile-time optimizations: inlining, constants extracting, pre-compiled visitors, read-only detection...
* runtime optimizations: typed vectors, SIMD, script allocators, stack variables...

As you see, compilation coupled with static code analysis opens the gates to safe code and efficiency.
 

```tip
Always treat KPHP as a compiled language — not as a tool, making PHP fast. This makes limitations obvious.  
Fitting best practices, you can achieve great runtime speed — as if your code was written directly in C++.
```
