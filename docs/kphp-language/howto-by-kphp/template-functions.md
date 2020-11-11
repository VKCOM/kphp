---
sort: 2
title: Template functions
---

# Template functions, duck typing

Consider the following code:
```php
function printIdOf($some) {
  echo $some->id;
}

printIdOf(new User);
printIdOf(new Message);
```

Of course, this simple snippet works fine in PHP, but it is incorrect from the type system point of view. 

What is the type of *$some*? How to declare *@param*? Do we want to work if passed *WallPost* for example?

Since union types for instances are not supported, appropriate solutions are:
* Make *User* and *Message* implement a common interface, *getId()* as a getter — and *printIdOf()* accepts it
* Extend *User* and *Message* from a basic class with *id* field — and *printIdOf()* accepts it
* Create copies of function to accept *User* and *Message* separately
* Use template functions

In fact, we want something like this, like in other compiled languages:
```
// pseudocode
function printIdOf<T>(T $some) { ... }
```

But PHP doesn't support templates. It's OK: PHP does not need it. **But we need strict typing.**

```tip
To solve this, **template functions** are supported. 
Template arguments are described in PHPDoc (see below) — for PHP it is just a comment and still valid code.
```


## @kphp-template doc tag

```php
/**
 * @kphp-template $some
 * @param object $some  
 */
function printIdOf($some) {
  echo $some->id;
}

// now compiles (if these classes have $id field of course)
printIdOf(User::get());
printIdOf(Message::get());
```
This is a basic example of **duck typing**. 

Because *$some* is a template parameter, KPHP doesn't analyze *@param* for it. You can write any type you want, just for IDE (*object* like here, or a union type or a trait name).

Non-template arguments are still typed and parsed:
```php
/**
 * @param object $obj   any instance, as a template
 * @param float $coeff  float, a regular parameter
 * @kphp-template $obj
 */
function multiplyBy($obj, $coeff) { }
```

If you specify many variables after `@kphp-template`, they must be of equal types then. A tag `@kphp-template` can appear more than once:
```php
/**
 * @kphp-template $a $b  - they must be the same class
 * @kphp-template $c     - it can be of any other class
 */
function d($a, $b, $c) { }
 
d(new A, new A, new B);   // ok
d(new A, new B, new C);   // error
d(new B, new B, new B);   // ok
```                 

Methods of classes can also be declared `@kphp-template`.

```warning
Template specializations are for instances only (not for numbers/strings for example).  
Why? Because primitives are inferred, and instances should be known at the preliminary analysis step.
```


## @kphp-template and arrays

The argument is not required to be just an instance. It can be an array of instances. The only condition is that everything should compile after substitution.

```php
class A {
  public function log($prefix) { }
}
 
class B {
  public function log($prefix) { }
}
 
/** @return A[] */
function getAArray() { }
 
/** @return B[] */
function getBArray() { }
 
/**
 * @param string $prefix
 * @param array $loggableArray
 * @kphp-template $loggableArray
 */
function logAll($prefix, $loggableArray) {
  foreach ($loggableArray as $loggable) {
    $loggable->log($prefix);
  }
}
```


## Template functions vs interfaces

The example above can be easily rewritten without template functions:
```php
interface Loggable {
  function log();
}
 
class A implements Loggable { /* ... */ }
class B implements Loggable { /* ... */ }
 
/**
 * @param string $prefix
 * @param Loggable[] $loggableArray
 */
function logAll($prefix, $loggableArray) { /* ... */ }
```

It looks better. But templates are generally more powerful. For instance, PHP interfaces can't contain fields:
```php
/**
 * @kphp-template $maybe
 */
function maybeIsEmpty($maybe) {
  return !$maybe->hasResult;
}
```
If you want exactly this code to be compiled, templates are the only choice. But generally, interfaces and instance functions are more likely to be used.

```note
A template function is cloned for every specialization. It leads to binary growth, but avoids dynamic calls at runtime. Normally you shouldn't think about dynamic calls and RTTI, so using interfaces is a more general approach, but keep this in mind if a function is very high-loaded.
```


## @kphp-return tag 

If a template function returns an instance depending on a template parameter, it can't be expressed using type hint or *@return*. There is another tag:
```php
class UserWrapper {
  /** @var User */
  public $data;
}

class MessageWrapper {
  /** @var Message */
  public $data;
}

/**
 * @param object $wrapper
 * @return object
 * @kphp-template T $wrapper
 * @kphp-return   T::data
 */
function getData($wrapper) {
  return $wrapper->data;
}

getData(new UserWrapper)->user_id;  // ok
```
Without `@kphp-return` you can still get is work, but you'll need to extract `getData(new UserWrapper)` as a separate variable and write PHPDoc above it — because instances should be resolved in advance.

If a function returns a primitive, you don't need `@kphp-return`. Again, it's needed very rarely — to help compiler bind →methods() before type inferring. 

Available syntax inside this tag is: `T / T::prop / T[] / T::prop[]`. More examples can be found in KPHP tests. 


## Every specialization is a separate type inferring step

```php
/**
 * @kphp-template $instance
 * @param any $arg 'any' means 'automatically inferred'
 */
function log1($arg, $instance) { /* ... */ }
 
log1(1, new A);
log1('string', new B);
// log1<A> : (int, \A)
// log1<B> : (string, \B)
```
Template functions are cloned in advance, that's why type inferring works separately for clones.

```php

/**
 * @kphp-template $instance
 */
function another($instance) {
  $error_code = $instance->error_code;
}
```
Same here: *$error_code* will have inferred type the same as the exact class, can differ between specializations. That's the main distinction from interfaces, by the way.

```danger
Be careful when using **static function variables** inside template functions. 
It is one function in PHP, but KPHP clones a function for every specialization. 
```
