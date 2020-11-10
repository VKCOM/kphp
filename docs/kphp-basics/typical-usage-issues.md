---
sort: 5
---

# Typical usage issues

When you start experimenting with KPHP, you'll definitely encounter some obscure errors.
```tip
Consult this page whenever you find yourself thinking **"What's going on?"**
```

<hr>


<blockquote class="faq">I wrote incorrect code, but KPHP feels OK</blockquote>

Your code is probably **not reachable**. It is never called, and therefore not visible to KPHP.  
Please refer to [What PHP code exactly would be compiled?](../kphp-language/kphp-vs-php/reachability-compilation.md)


<blockquote class="faq">I get some strange unreadable error and <i>"Make failed"</i></blockquote>

```
  0% [total jobs 22] [left jobs 22] [running jobs 0] [waiting jobs 5]
In file included from /home/me/kphp/php_functions.h:9:0:
/home/me/kphp/runtime/array_functions.h:1203:42: error: no matching function for call to 'f$floatval(const std::tuple<long int, string>&)'
     return f$floatval(h1) > f$floatval(h2);
...

Compilation error [gen by /home/me/kphp/compiler/make/make.cpp at 399]
In stage = [Make]:
  [file = unknown]
  [function = unknown function:0]
  Make failed
```
The *...* error above is from *g++* (not from KPHP). C++ errors are always cumbersome :)

This means that you have written incorrect code (like using some strange operations, e.g. sorting tuples), but KPHP didn't detect this on the codegeneration phase. It's an error from *g++* while compiling C++ code.  
This is most likely a bug, as KPHP should have noticed it earlier and printed a more friendly error.
  
**Solution.** Usually, the root of the error is somewhere in the beginning, like `.../o_25/demoFunction.cpp:46:13: required from here`, followed by comprehensive g++ errors. Investigate the changes you made, especially in the mentioned functions. There would be something simple, but hard to find.
  
You're welcome to write down a minimal example and submit it as an issue.


<blockquote class="faq">I get an error <i>"change_user_group: can't find the user kitten to switch to"</i></blockquote>

When launching the server from **root**, KPHP switches to the user *kitten*, as working from root is bad practice.  
The error means that you don't have the user *kitten* in your system.

**Solution.** You can either create the user *kitten*: `useradd -ms /bin/bash kitten`, or explicitly allow root `./server -u root`


<blockquote class="faq">I call <code>standard_php_function()</code>, but get an error <i>"Unknown function"</i></blockquote>

This means that KPHP does not have this function implemented yet. As stated before, KPHP supports many PHP functions, but not all of them.

**Solution.** Write and call a function in PHP that does what you need. Optionally, you can create a relevant issue on [Github]({{site.url_github_kphp}}).


<blockquote class="faq">I get an error <i>"Using Unknown type"</i></blockquote>

This can occur in two cases.
1. You use undefined variables:
```php
function demo() {
  echo $asdf;  // it was never assigned, it can't be inferred and has Unknown type
}
```

2. You get an element from an always empty array
```php
function demo($options = []) {     // if demo() is ALWAYS called with an empty array,
  if ($options['use_wrapper']) {}  // then $options is Unknown[]
}
```
Use [PHPDoc](../kphp-language/static-type-system/phpdoc-to-declare-types.md) to declare array type (*@param bool[] $options* for example).


<blockquote class="faq">KPHP shows something about <code>Error</code> type</blockquote>

```
TYPE INFERENCE ERROR:
Expected type:  array< Unknown >
Actual type:    Error

STACKTRACE:
...
```

The *Error* type comes up when KPHP can't mix two types fitting the type system. Some examples:
* You mix different classes in the same variable, and they don't have a common ancestor.
* You pass tuples with different sizes to the same function argument.
* You declare *@return array*, but return a *shape*.

**Solution.** Follow the stacktrace, as it usually points backward an *Error* type occurred.


<blockquote class="faq">I get <i>"!==, ===, is_array or similar may differ from PHP"</i></blockquote>

This happens when you read from typed arrays: reading from *array\<T\>* at non-existing element returns *T*, not *null*, that's why *===null* and similar would work differently.

**Solution.** Use *not_null()* when getting from an array, for details consider [this article](../kphp-language/static-type-system/typed-arrays.md#how-to-deal-with-this-error--result-may-differ-from-php).


<blockquote class="faq">I pass a string, but KPHP infers <code>mixed</code></blockquote>

```php
function demo(int $x) {}
demo('asdf');
```

This leads to an error *"Actual type: mixed"*, while you might be expecting *"Actual type: string"*.

This is because given *int* works as apriori information, saying *"$x is at least int"*. When a string is passed, *int* and *string* together bring out *mixed*.

All in all, when there are no compilation errors, types fully coincide with those manually specified. When there are errors, actual types combine with manually specified, which may be a bit confusing.


<blockquote class="faq">I want to pass different objects to a function, but get <i>"mix classes"</i> error</blockquote>

```php
function printIdOf($object) { echo $object->id; }
printIdOf(new A);
printIdOf(new B);
```

This leads to *Error* with the message *"mix classes A and B"*, because type of *$object* can't be inferred. The same occurs when placing different classes into a single array: `[new A, new B]` — this is *array\<Error\>*.

**Solutions.**
1. Implement a common interface, so that *demo()* would accept this interface as a parameter
2. Use [template functions](../kphp-language/howto-by-kphp/template-functions.md) 


<blockquote class="faq">I specify PHP 7 class type hint, but KPHP doesn't prevent from passing <code>null</code></blockquote>

Currently, KPHP doesn't track nullability. If you declare `function f(A $a)`, you can pass a nullable *A*, which will trigger a fatal error in PHP. In KPHP, instances can store null values at runtime.

**Solution.** Use *?A* instead of *A* or use *@param* instead of type hint.


<blockquote class="faq">KPHP shows only one type of mismatch error, I want to see all of them</blockquote>

When invoking *kphp*, use `--show-all-type-errors` option or `KPHP_SHOW_ALL_TYPE_ERRORS=1` env variable.

One source of error can often trigger mismatches as a chain reaction, that's why this is set to *false* by default.


<blockquote class="faq">Are <code>mixed[]</code> and <code>array</code> the same or not?</blockquote>

They are different: *array* means **an array of anything** (what would be automatically inferred, such as *int[]*, or *SomeClass[]*, or *mixed[]*), but *mixed[]* is an array of *mixed*.

When you use *array* type hint, specifying *@param T[]* above is good practice.

See the explanation about *any* type [here](../kphp-language/static-type-system/kphp-type-system.md#any).


<blockquote class="faq"><code>json_encode($object)</code> doesn't compile, as well as <code>serialize($object)</code></blockquote>

Yes, *json_encode()*, *var_dump()*, *print_r()*, etc. accept *mixed*, so instances can't be passed there.

**Solution.** Read about [JSON manipulation](../kphp-language/howto-by-kphp/json-encode-decode.md) and [serializing instances](../kphp-language/howto-by-kphp/serialization-msgpack.md).


<blockquote class="faq"><code>function ... use(&$by_reference)</code> doesn't compile</blockquote>

References in *use* are not supported.  
References are supported in two cases only:
* function argument as a reference
* foreach by reference

**Solution.** Rewrite your code without use-references or create a wrapper instance and capture it in a closure.


<blockquote class="faq">Any solutions to call a function by name?</blockquote>

Generally, there's nothing better than *switch-case* that can done in compiled languages.

For complex scenarios, consider codegenerating *switch-case* from a schema. Continue reading [here](../kphp-language/howto-by-kphp/call-function-by-name.md).


<blockquote class="faq">KPHP doesn't understand <code>trigger_error()</code></blockquote>

Yes, this function doesn't exist in KPHP.

**Solution.** Use *warning($message)* instead (it is [polyfilled](../kphp-language/php-extensions/php-polyfills.md) with *trigger_error()* in plain PHP).


<blockquote class="faq"><code>ini_set('memory_limit')</code> compiles, but fails executing</blockquote>

It makes no sense to KPHP: like many other *ini_set()* arguments, *'memory_limit'* is unknown.

**Solution.** Surround this call with *#ifndef* to make it invisible ([explanation](../kphp-language/kphp-vs-php/reachability-compilation.md#ifndef-kphp)):
```php
#ifndef KPHP
ini_set('memory_limit', '512M');
#endif
```
 

<blockquote class="faq">I face behavior differences between PHP and KPHP</blockquote>

KPHP pretends to behave exactly the same as PHP, but you may encounter misbehavior. This usually occurs in very strange or rare usage cases, which are not covered by tests.

**Solution.** Rewrite that piece of code with different branches for PHP and KPHP — using *#ifndef* or *if(kphp)*. A submitted issue with steps to reproduce will be appreciated.
