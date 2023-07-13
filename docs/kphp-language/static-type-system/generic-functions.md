---
sort: 7
title: Generic functions f&lt;T&gt;()
---

# Generic functions f\<T\>()

```note
Many languages have generics, but PHP has not: you just pass/store anything/anywhere.  
In KPHP, you should deal with the type system, like in any other compilable language.
```

KPHP brings full support of generic functions, including primitives, auto-reification, guards, and special PHP instantiation syntax.


## The need of generics for strict typing

Any function accepts typed arguments. If classes *A* and *B* both have a method *doSmth()*, and we want to write
```php
function f($obj) {
  $obj->doSmth();
}
```

Then how to express `@param $obj`?  
* If *A* and *B* implement the same interface having such method — then *$obj* is *I*.  
* If *A* and *B* extend the same base class having such method — then *$obj* is *Base*.  
* But if *A* and *B* have nothing in common, *$obj* doesn't have a valid type, since `A|B` is invalid.

Another example: about custom `array_map()` implementation. How to declare it?
```php
function my_map(array $a, callable $cb) { 
  $result = [];
  foreach ($a as $k => $v)
    $result[$k] = $cb($v);
  return $result; 
}
```

Since it's a regular function, and the types are auto inferred unless declared, as a union of all available cases:
```php
my_map([1,2,3], ...);   // $a: int[]
my_map(['str'], ...);   // $a: string[], combined with int[] => inferred as mixed[]
my_map([new A], ...);   // $a: A[], combined with mixed[] => compilation error
```

Generic function solve these problems. You expose these functions as generic:
```php
// pseudocode
function f<T>(T $obj): void;
function my_map<TElem, TOut>(TElem[] $a, callable(TElem):TOut $cb): TOut[];
```


## What is a generic function

As you've seen above, a generic function allows you to have duck typing. In pseudocode, you'd write:
```php
// pseudocode
function f<T>(T $obj) {
  $obj->duckTyping();
}
```

There may be many Ts, and parameters can depend from Ts in any way:
```php
// pseudocode
function filterGreater<T>(T[] $arr, int $level=0) { ... }
function acceptTuple<T1,T2>(tuple(T1,T2) $t) { ... }
```

When you call such functions, you either assume a compiler to guess Ts, or provide them manually:
```php
// pseudocode
f(new A);                   // calls f<A>
f<A[][]>([]);               // calls f<A[][]>
filterGreater($a->ints);    // calls filterGreater<int>
filterGreater<string>([]);  // calls filterGreater<string>
```

When you operate generic functions in KPHP, think of them in exactly the same way. But keep in mind, that syntax is invalid for PHP, that's why
* in declaration, you write `@kphp-generic T` instead of `f<T>`
* in declaration, you write `@param T[] $arr` instead of `T[] $arr`
* in a call, you write `/*<string>*/` instead of `<string>`


## @kphp-generic, @param and @return

Let's write `eq()` function that takes any value and returns itself:
```php
/**
 * @kphp-generic T
 * @param T $value
 * @return T
 */
function eq($value) {
  return $value;
}
```

It's exactly what we want to express:
```php
// pseudocode
function eq<T>(T $value): T {
  return $value;
}
```

Note, that regular *@param/@return* tags are used, not *@kphp-param* or similar — because having *@kphp-generic* written, `T` is parsed as generic T, not as class T.

When calling `eq()`, T could be any type (an instance, an array of something, a primitive, etc.):
```php
class A {
  public int $i;
  /** @var tuple(int, string) */
  public $t;
}

eq($a);             // eq<A>
eq(['s']);          // eq<string[]>
eq($a->t);          // eq<tuple(int,string)>
eq([getA()]);       // eq<A[]>
```


## @var can contain T

In a function body, `T` is also a valid type inside PHPDocs, in `@var` particularly.

```php
/**
 * @kphp-generic T
 * @param T $obj
 * @return T
 */
function my_clone($obj) {
  /** @var T */
  $clone = clone $obj;
  $clone->afterClone();
  return $clone;
}
```


## T[], callable(T), etc.

Let's write `my_array_filter()`:
```php
/**
 * @kphp-generic T
 * @param T[] $arr
 * @param callable(T):bool $callback
 * @return T[]
 */ 
function my_array_filter($arr, $callback) {
  /** @var T[] */
  $result = [];
  foreach ($arr as $k => $v)
    if ($callback($v))
      $result[$k] = $v;
  return $result;
}
```

Here T is not the type of an argument, but the type of array elements, and also is used in a callable. `my_array_filter<int>` accepts `int[]`, and so on.

Another example:
```php
/**
 * @kphp-generic T
 * @param future<T>[] $resumables
 * @return T[]
 */
function wait_all(...$resumables) {
  $answers = [];
  foreach ($resumables as $key => $resumable)
    $answers[$key] = wait($resumable);
  return $answers;
}
```


## Auto-reification

When you call any generic function, KPHP deduces T at exact call. Decucing generic types is called "reification".

```php
my_array_filter([1,2,3], fn($i) => $i < 3);         // T=int, return=int[]
my_array_filter([getA()], function($a) {            // T=A, return=A[]
  return $a->isValid();   // KPHP understands that $a is A because of callable(T)
});
my_array_filter([[1]], fn($ar) => count($ar)>0);    // T=int[], return=int[][]
```

In short, to perform reification, we use assumptions: having `T $arg` and `f(any_expr)`, calc `T=(assumption of any_expr)`. That's why it perfectly works for instances and partially works for primitives:
```php
f($a)           // T=A
f(1)            // T=int
f(getIntArr())  // T=int[]
f($obj->field)  // T=@var above field
f($int)         // can't reify T unless explicit @var set, because assumptions for vars-primitives aren't stored
```

Sometimes, T can't be automatically detected:
```php
my_array_filter(1, ...);            // can't reify T, argument is not an array
my_array_filter([], ...);           // can't reify T from empty array
eq(null);                           // can't reify T, not enough information
```


## Manual /\*\<...\>\*/ providing syntax

When T can't be auto reified, a PHP programmer can provide them manually:
```php
my_array_filter/*<int>*/([], ...);          // [] is int[]
my_array_filter/*<A>*/([], fn($a) => $a);   // [] is A[], $a is A
eq/*<A>*/(null);                            // null treated as A (instances are nullable)
eq/*<?int>*/(null);                         // null treated as ?int
eq/*<int>*/(null);                          // error on type checking: passing null to int
```

So, for PHP it's just a regular comment and doesn't affect behavior (and highlighted as a comment in IDE), but KPHP treats it as a special syntax.

Sometimes, `/*<...>*/` could be used intentionally even if it can be reified, to make your code more readable.

Inside generic functions, T is also allowed inside these brackets when calling another generic function:
```php
/**
 * @kphp-generic T
 * @param T $obj
 */
function firstGenericFn($obj) {
  anotherGenericFn/*<T>*/([]);
}
```


## Multiple Ts at declaration

`@kphp-generic` lists all Ts, separated by comma. Say, we want to implement such a function
```php
// pseudocode
function ofSameClass<T1, T2>(T1 $o1, T2 $o2): bool {
  return $o1 && $o2 && get_class($o1) === get_class($o2);
}
```

We do pretty much the same using PHPDoc syntax:
```php
/**
 * @kphp-generic T1, T2
 * @param T1 $o1
 * @param T2 $o2
 */
function ofSameClass($o1, $o2): bool {
  return $o1 && $o2 && get_class($o1) === get_class($o2);
}
```

As earlier, they are auto-reified:
```php
ofSameClass(new A, new B);      // ofSameClass<A, B>
ofSameClass(new A, null);       // error: T2 can't be reified
```

A `/*<...>*/` syntax also accepts a comma:
```php
ofSameClass/*<A, B>*/(new A, null);
```


## extends condition \`T: SomeInterface`

We can restrict any T to deny passing arbitrary classes, but still leave a function generic:
```php
/**
 * @kphp-generic T: SomeInterface
 * @param T $obj
 */
function f($obj) { ... }

f(new Impl1);   // ok
f(new A);       // error, A doesn't fit the condition
```

A condition to the right of ':' doesn't have to be a valid type from the point of view of type system: it's a guard, not a type. Hence, you can mix unrelated interfaces, the only criteria is that your code is still compiled after generics instantiation:
```text
@kphp-generic T: ISignalParam | IApiParam
```

A condition may also contain primitives — for example, to allow only clean types, not mixed (as you remember, every generic instantiation is compiled to a separate C++ function):
```text
@kphp-generic T: int|string|float|null
@kphp-generic T: Stringable | string
```

When multiple Ts, any of them can contain a separate condition:
```text
@kphp-generic TKey, TValue: IValue
@kphp-generic TKey: int|string, TValue: IValue
```


## Default value \`T = def`

Besides extends hint, there is a '=' syntax to provide a default hint:
```text
@kphp-generic T1, T2 = mixed   // T1 required, T2 has a default
```

It will be especially useful with generic classes in the future:
```text
@kphp-generic T, TCont = Container<T>
```

When both extends and default exist, a default must be the last:
```text
@kphp-generic TValue, TErr : \Err = \Err
```

Default types are applied when T can't be reified and is not provided manually.
```php
/**
 * @kphp-generic T = \Exception
 * @param ?T $obj
 */
function logMessage($obj = null) {
  if ($obj !== null)
    echo $obj->getMessage(), "\n";
}

logMessage(null);       // logMessage<Exception>
```

When multiple Ts, the latter T may have a default depending on the earlier:
```text
@kphp-generic T1, T2 = T1, T3 = T2|T1
```

Default Ts can be omitted on a call size with `/*<...>*/` syntax:
```php
fWithT123/*<T1>*/();        // T2 and T3 from default
fWithT123/*<T1, T2>*/();    // T3 from default
```


## \`callable` keyword

A `callable` argument actually implicitly turns a function into generic:
```php
function f(callable $cb) {}

// is the same as
/**
 * @kphp-generic CallbackT: callable
 * @param CallbackT $cb
*/
function f($cb) {}
```

`callable` as extends hint means that plain objects / primitives can't be passed to a function.


## \`object` keyword

KPHP supports `object` keyword for arguments that acts like untyped callable, turning a function into generic:
```php
function f(object $obj) {
  $obj->duckTyping();
}

// is the same as
/**
 * @kphp-generic T: object
 * @param T $obj
 */
function f($obj);
```

`object` as extends hint means that a type must be any instance, it can't be a primitive, an array, etc.

Besides a PHP keyword, you can of course use `T:object` manually, disallowing passing primitives:
```php
/**
 * @kphp-generic T: object
 * @param T[] $arr_of
 */
function f($arr_of);
```


## class-string\<T\>

Some built-in functions accept `A::class` and return `A`. We can describe the same behavior using generics.

Say, we want to implement `my_cast()` that performs checks:
```php
// pseudocode
function my_cast<T, DstClass>(T $obj, ??? $class_name): DstClass {
  $casted = instance_cast($obj, $class_name);
  if ($casted === null)
    throw new Exception;
  return $casted;
}
```

We want it to be used as `my_cast($obj, B::class)` so that it returns `B`, as `$class_name` is a compile-time information during instantiation.

Here's how we can do this:
```php
/**
 * @kphp-generic T, DstClass
 * @param T $obj
 * @param class-string<DstClass> $class_name
 * @return DstClass
 */
```

`Some::class` is auto-reified as `Some` when matched by `class-string<T>`.

Here is another example:
```php
/**
 * @kphp-generic T
 * @param class-string<T> $cl
 * @return T[]
 */
function emptyArrayOf($cl) {
  return [];
}

emptyArrayOf(A::class);     // A[]
emptyArrayOf(SomeI::class); // SomeI[]
```


## classof($obj)

If `class-string<T>` is a **TYPE** providing compile-time information, `classof($obj)` is an **EXPRESSION** providing compile-time information if type of `$obj` is compile-time known at the point we are instantiating generics.

`classof($obj)` is a keyword returning `class-string<>`, so it's possible to do this:
```php
/**
 * @kphp-generic T: object
 * @param T $obj
 * @return T[]
 */
function emptyArrayLike($obj) {
  return emptyArrayOf(classof($obj));
}
```

Another example:
```php
function getSignalByName(string $signal_name): ISignal { ... }

/**
 * @kphp-generic T: ISignal
 * @param T $arg
 * @param callable(T):void $handler
 */
function addHandler(string $signal_name, $arg, $handler) {
  $signal = getSignalByName($signal_name);            // ISignal
  $casted = instance_cast($signal, classof($arg));    // T
  $arg->mergeWith($casted);
  $handler($arg);
}

addHandler('f1', new Impl1, function(Impl1 $p) { ... });
addHandler('f1', new Impl2, function(Impl2 $p) { ... });
```


## Auto-reification from callable return

Now it's possible to create `my_map()` function:
```php
/**
 * @kphp-generic TElem, TOut
 * @param callable(TElem):TOut $callback
 * @param TElem[] $arr
 * @return TOut[]
 */
function my_map($arr, $callback) {
  /** @var TOut[] */
  $result = [];
  foreach ($arr as $k => $v)
    $result[$k] = $callback($v);
  return $result;
}
```

KPHP will not only infer *TElem*, but also *TOut* (as a return value of a lambda) when calling it:
```php
$strs = my_map([1,2,3], fn($i) => (string)$i);      // my_map<int, string>
foreach (map(getArr(), fn($i) => getA($i)) as $a)   // my_map<int, A>
  $a->aMethod();
```


## @return T::field

Suppose you have several classes
```php
class ValueInt { public int $value; }
class ValueArrayString { /** @var string[] */ public $value; }
// ...
```

And you want to declare
```php
// pseudocode
function getValueOf<T>($obj): ??? {
  return $obj->value;
}
```

What *@return* should be like? We want to express *"a function returns a type that T::value has"*.
```php
/**
 * @kphp-generic T
 * @param T $obj
 * @return T::value
 */
function getValueOf($obj) {
  return $obj->value;
}
```


## @return T::method()

The same as above, but
```php
class ValueInt { public getValue(): int { ... } }
class ValueArrayString { /** @return string[] */ public getValue() { ... } }
```

We want to express *"a function returns a type that T::getValue() returns"*.
```php
/**
 * @kphp-generic T
 * @param T $obj
 * @return T::getValue()
 */
function getValueOf($obj) {
  return $obj->getValue();
}
```


## Variadic generics f\<...T\>

You can declare a function that accepts arbitrary number of arguments as a variadic parameter:
```php
/**
 * @kphp-generic ...T
 * @param T ...$args
 */
function f(...$args) { }  
```

Internally, when `f()` is called with N arguments, `$args` is replaced with N copies. E.g., N=2:
```php
f(new A, new B);        // N = 2
// actually creates
function f_n2<T1, T2>(T1 $args_1, T2 $args_2) {
  // body is cloned, $args is replaced with flat copies
}
// and calls
f_n2<A, B>(new A, new B);
```

There is a limited number of valid operations you can perform over `$args`:
* forward to another function via `g(...$args)`
* concat into an array via `[...$args]`

More precisely, `$args` is replaced with N copies of arguments, and this replacement 
is correct in a limited number of constructions.

Variadic generics are useful to express proxy functions that pass parameters to other functions as-is or with slight modifications. 
Here are examples of simple forwarding, appending and prepending:
```php
/** 
 * @kphp-generic ...TArg
 * @param TArg ...$args
 */
function forwardToG(...$args) {
  log("call g n=" . count($args));
  g(...$args);      // actually replaced by g($args_1, $args_2, ... args_N)
}

/**
 * @kphp-generic ...TMore
 * @param TMore ...$more
 */
function prependIntToG(int $first, ...$more) {
  g($first, ...$args);
}

/** 
 * @kphp-generic T, ...TArg
 * @param T $append
 * @param TArg ...$args
 */
function appendSomethingToG($append, ...$args) {
  g(...[...$args, $append]);
  // actually replaced by g(...[$args_1, $args_2, ... args_N, $append])
}
```

Note, that `g()` is not required to accept a variadic parameter, because unpacking a fixed-size array 
to positional arguments is valid both in PHP and KPHP. 
```php
function g(int $a, int $b, int $c = 0) {}

// ok, g(1, 2, 0)
g(...[1,2]);

// ok, g(...[$args]) for N=3 is g(...[$args_1, $args_2, $args_3])
// correctly unpacked to g($args_1, $args_2, $args_3)  
forwardToG(1, 2, 3);
```

A `/*<...*>/` syntax also works with variadic generics and helps in cases when Ts can't be auto reified:
```php
variadicF/*<A>*/(null);                         // actually, variadicF_n1<A>
variadicF/*<A, B, ?int>*/(null, null, null);    // actually, variadicF_n3<A, B, ?int>
```


## new $class, $class::method(), etc.

PHP allows to create a class by name / call a method by name. Generally, KPHP does not, but in case of generics, it works. 
If `$class` is a compile-time known `class-string<T>`, it's possible to use such constructions:
```php
/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function demo($class_name) {
  $obj = new $class_name;         // ok
  $obj->afterCreated();
  $class_name::staticMethod();    // also ok
}

demo(A::class);     // actually, demo<A>('A')
demo(B::class);     // actually, demo<B>('B')
```

Here it's possible **because $class is compile-time known**, everything is statically resolved.

This gives you an ability to write wrappers around constructors:
```php
/**
 * @kphp-generic TCreated
 * @param class-string<TCreated> $cn
 * @return TCreated
 */
function createAndCheckValid(string $cn, int $arg) {
  $obj = new $cn($arg);
  if (!$obj->isValid()) {
    throw new RuntimeException("Object($cn) is not valid for arg=$arg");
  }
  return $obj;
} 
```

You can pass here any classes accepting `int` in constructor and having `isValid()` method.

Combined with variadic generics this feature becomes much more powerful:
```php
class A {
  function __construct() { ... }
  function init() { ... }
}

class B {
  function __construct(int $a) { ... }
  function init() { ... }
}

class C {
  function __construct(A $a, ?B $b) { ... }
  function init() { ... } 
}

/**
 * @kphp-generic T, ...TArg
 * @param class-string<T> $class_name
 * @param TArg ...$args
 * @return T
 */
function createAndInit($class_name, ...$args) {
  $obj = new $class_name(...$args);
  $obj->init();
  return $obj;
}

createAndInit(A::class);
createAndInit(B::class, 10);
createAndInit(C::class, new A, new B(0));
```

If generic Ts can't be auto-reified by arguments, they could be provided manually, as before:
```php
// without a hint, it's impossible to guess what 'null' means
createAndInit/*<C, A, B>*/(C::class, null, null);
```
   
Same for wrappers around calling any static method:
```php
/**
 * @kphp-generic T, ...TArg
 * @param class-string<T> $class_name
 * @param TArg ...$args
 */
function calcAndCheckGreater0($class_name, ...$args): int {
  $res = $class_name::calcCoeff(...$args);
  if ($res <= 0) {
    log("Unexpected coeff=$res for $class_name");
  }
  return $res;
} 
```

These syntax constructions would work:
* `new $class` (probably with arguments)
* `$class::staticMethod()` (probably with arguments)
* `$class::CONST`
* `$class::$STATIC_FIELD` (even for writing)

`$class::instanceMethod()` does not work, and should not.  
`$class::$dynamic_method()` also does not, a method name should be compile-time known.


## A problem with null

KPHP tries no auto-reify Ts when you pass arguments to generic functions. `f(3)` is reified as `f<int>(3)`, and similar. 
But KPHP can do nothing about `f(null)`, since `null` is not a type, it's a flag of any null-containing type.

In case you want to pass nulls, you should manually provide `/*<...>*/` at the point of instantiation:
```php
f/*<?int>*/(null);
f/*<mixed>*/(null);
f/*<A[]|null|false>*/(null);
```

If `null` is one of the arguments, and others are correctly reified, you should nevertheless pass all Ts manually, 
since it's the awaited syntax:
```php
f/*<int, A, ?int>*/(0, new A, null);
```

If this becomes a frequent case, we could introduce something like `f/<auto, auto, ?int>*/` or invent something better 
by integrating `null` into the type system.

By the way, `true` and `false` don't have such ambiguities, they are reified as `bool`.


## A problem with primitives and variables

The point when we are instantiating generics is exactly the same point when we bind `f()` and `$a->f()` calls to real functions. In order to bind `$a->f()` to a function, we need to know the type of `$a`. To manage this, we use assumptions, which are based on variable names. For instance, if `@param A $a` exists, we know that *$a* is *A*.

All assumptions are calculated BEFORE type inferring. It's supposed that type inferring would afterward produce the same types. For instances, we can analyze simple expressions and use PHPDoc, but for primitives, it's generally wrong, as they can be splitted and smartcasted:
```text
// php
function f(mixed $id) {
  $id = (int)$id;
  echo $id;
}

// c++
void f$f(mixed $id) {
  $id$v1 = (int)$id;
  echo $id$v1;
}
```

Here's the problem:
```php
function f(mixed $id) {
  $id = (int)$id;
  eq($id);    // (1)
}
```

In (1), when instantiating generics, we can only rely on variable names, we don't know anything about vars split and smart casts, but taking *mixed* from an argument is wrong, as it would become *int* at that point later.

KPHP fires an error in all cases where primitives can potentially change their types:
```text
Couldn't reify generic <T> for call.
Please, provide all generic types using syntax eq/*<T>*/(...)
```

The action you need to perform is to manually provide T:
```php
function f(mixed $id) {
  $id = (int)$id;
  eq/*<int>*/($id);   
  eq/*<mixed>*/($id); // or maybe you want this
}
```

Another example:
```php
function f() {
  $id = ... ? 0 : null;
  eq($id);    // error, a variable with a primitive
  if ($id)
    eq($id);    // because it can be smart casted, like here
}
```

Such problem exists only with variables that contain primitives. For example, if a function returns a primitive or a field contains a primitive, everything works as expected, because they can't be smart casted:
```php
eq(getInt());       // ok, eq<int>
eq($obj->str);      // ok, eq<str>
eq(8.4);            // ok, eq<float>
eq([1,2]);          // ok, eq<int[]>

$int = getInt();
eq($int);           // but it's an error
```

Nevertheless, as a special workaround, a PHP programmer can write `@var` to explicitly specify a type, then reification will find and use it:
```php
/** @var int $int */
$int = getInt();
eq(int);            // ok, eq<int> because of @var
```

By the way, lambdas are the only exception, variables are taken from assumptions even regardless of smart casts, because in practice, it's very uncommon whey would be smart casted inside lambdas:
```php
// it's map<int, int>, the second 'int' is a primitive-variable, but for lambdas it works
map([1,2,3], fn($i) => $i);
```

All in all,
* passing instances works well
* passing constants works well
* passing fields/functions works well
* passing variables (only with primitives) is tricky, but `/*<T>*/` or `@var` helps


## Constexpr switch

Suppose you want something like `$factory->getInstance<T>()`, to be used like
```php
$factory->getInstance(FooService::class)->fooMethod();
$factory->getInstance(AnotherService::class)->anotherMethod();
```

Here is a possible implementation:
```php
/**
 * @kphp-generic TService
 * @param class-string<TService> $serviceClass
 * @return TService
 */
function getInstance(string $serviceClass) {
  switch ($serviceClass) {
  case FooService::class:
    return $this->foo ?? ($this->foo = $this->createFoo());
  case AnotherService::class:
    return $this->getAnother();
  // more services
  }
  
  if (isset(self::HIDDEN[$serviceClass])) {
    throw new Exception("$serviceClass is private and can not be used");
  }
  throw new Exception("$serviceClass not found");
}
```

It will work as expected, but to make it valid, KPHP performs a skilful trick. Let's see how `getInstance<FooService>` would look like:
```php
function getInstance<FooService>(string $serviceClass = 'FooService'): FooService {
  switch ($serviceClass) {
  case FooService::class:
    // return FooService object, ok
  case AnotherService::class:
    // return another object, ERROR! types mismatch, FooService expected
  }
  ...
}
```

As you see, without any modifications, the code is erroneous. 

But we have a guarantee that `$serviceClass` is 'FooService' in this cocrete generic specialization.
In other terms, it's a **constexpr** compile-time known string. We have a switch over a constexpr string,
so the only case is valid, which is also calculated at compile-time, so `switch` itself could be just replaced 
with a body of a true `case`, or `default` if all cases are false. Also, `$serviceClass` variable can not be modified.

KPHP automatically performs such transformations for `switch` over constexpr a string, with constexpr cases,
where every non-empty case ends with break/return/throw. Constructions like `if/else` are not analyzed,
so use such pattern for your scenarios.


## Generic methods are allowed

Both static methods and instance methods could be made generic.
```php
class Utils {
  /**
   * @kphp-generic T1, T2
   * @param T1 $o1
   * @param T2 $o2
   */
  static function eq($o1, $o2): bool { ... }
}

Utils::eq(new A, new B);
Utils::eq/*<A, B>*/(null, null);

class StringVec {
  /** @var string[] */
  public int $items;

  /**
   * @kphp-generic T
   * @param T[] $arr
   */
  function joinWith(array $arr) {
    foreach ($arr as $a)
      $this->items[] = (string)$a;
  }
}

// there will be 4 actual methods emitted after generics instantiation
$vec->joinWith(['1']);
$vec->joinWith([new AHasToString]);
$vec->joinWith([7]);
$vec->joinWith/*<?int>*/([]);
```

`self` can be used as extends hint and default hint for generic Ts.


## Generic methods and inheritance

Interfaces can have generic methods, which may be overridden in child classes (and must also be generic). Since `@param/@return` are auto inherited, they could be omitted in child classes.
```php
interface StringableWith {
  /**
   * @kphp-generic T
   * @param T $with
   */
  function echoSelfWith($with);
}

class A implements StringableWith {
  /** @kphp-generic T */
  function echoSelfWith($with) {
    // $with is T
    echo "A and ", stringify($with), "\n";
  }
}
```

Note, that regardless of PHPDoc inheritance, `@kphp-generic` is required for child methods. If omitted, it won't be treated as a generic function and will lead to compilation errors (inheriting a non-generic from generic).


## KPHPStorm

[KPHPStorm](../kphpstorm-ide-plugin/README.md) correctly handles T in PHPDocs, understands the `/*<...>*/` syntax, automatically collapses such fragments, auto-reifies Ts according to the same rules to provide inferring, even for callables, warns if there is not enough information for reification, etc.


## Generic classes like Container\<T\>

Proceed to the next page: [Generic classes](./generic-classes.md).
