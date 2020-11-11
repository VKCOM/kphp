---
sort: 2
title: PHPDoc to declare types
---

# PHPDoc is the way you declare types

We have already discussed, that if you omit types, KPHP will infer them.  
But teach yourself to write types manually — and have more intelligent code with compiler type checks.

There are 2 ways to declare types:
* using PHPDoc — you can express almost any supported type
* using PHP 7 type hints — a very limited syntax, but suitable for simple cases


## PHPDoc: @var, @param, @return

Let's start with an example:
```php
/**
 * @param int|false $user_id if false, current id used
 * @return User|null
 */
function loadUser($user_id = false) {
  if (!$user_id) {
    $user_id = getCurrentUserId();  
  }
  $db_response = executeQueryLoadingUser((int)$user_id);
  if (!is_array($db_response)) {
    return null;
  }
  return User::createFromDbResponse($db_response);
}

loadUser($_GET['user_id']);        // (1) error! passing mixed to int|false
loadUser((int)$_GET['user_id']);   // (2) correct
```

Without PHPDoc, line (1) will be OK, just argument *$user_id* will be inferred as `mixed`.  
With PHPDoc, you get an error on (1). This means: if successfully compiled, all types satisfy declarations.

*@return* is also checked:
```php
/**
 * @return float[]
 */
function getCoefficients() {
  $coeff = [1, 2.5];
  if (isset($_GET['multiplicator'])) {
    $coeff[0] *= $_GET['multiplicator'];  // turns to mixed[]
  }
  return $coeff; // error
}
```

This will lead to a compilation error, as *$coeff* will be inferred `mixed[]`, whereas function declared `float[]`.  
Without PHPDoc, *getCoefficients()* will be just inferred `mixed[]`. PHPDoc protects you from type spoiling.

This code can be corrected like this:
```php
    $coeff[0] *= (float)$_GET['multiplicator'];
```

When you declare classes, *@var* is used to declare fields: 
```php
class User {
  /** @var ?int */
  public $id = null;
  /** @var string */
  public $name = "";
  /** @var User[] cached already loaded users: map [id=>User] */
  static private $cache = [];
}
```
And again, if you assign incorrect types to fields — PHPDoc types won't match inferred — you'll get an error. If compiled, fields' types are just the same as been written.

Though optional, you can use PHPDoc for local vars / global vars / static vars:
```php
function some() {
  /** @var float[] $coeff */
  $coeff = [];
  $coeff[] = 1;     // ok
  $coeff[] = "3.5"; // error
}

/** @var int|false $CurrentId */
$CurrentId = false;
```

PHPDoc for a local var prevents it from splitting:
```php
function some() {
  /** @var string $hash */
  $hash = (string)$_GET['hash'];
  /* ... */
  $hash = calcAnotherHashOrNull();  // error: ?string assigned to string
}
```

Without PHPDoc, *$hash* will be split to 2 variables — *string* and *?string* (described earlier). You can split manually: 
```php
function some() {
  /** @var string $hash */
  $hash = (string)$_GET['hash'];
  /* ... */
  /** @var ?string $hash */
  $hash = calcAnotherHashOrNull();  // ok 
}
```
But generally speaking, using one name for logically different variables with different types is a bad practice.

If *@var* PHPDoc is just above assignment to local var, *$var_name* can be omitted, as it is clear from context:
```php
/** @var int|false */
$pos = strpos("hello", "h");
```

You can mix comments with types:
```
@param {type} $var_name optional comment
@return {type} optional comment
@var {type} optional comment
@var {type} $var_name optional comment
```

Types and var names can be swapped, but not recommended:
```
// swapping not recommended! mostly for compatibility with existing code
@param $var_name {type} optional comment
@var $var_name {type} optional comment
```

That's why you can't just write comments (without types), as first part of comment will be treated as type:
```
// wrong! comment will be parsed as {type} and most likely end up with error
@param $var_name comment
```

When a function has multiple arguments, specify *@param* for every of them separately:
```php
/**
 * @param int $a
 * @param int $b
 * @return int
 */
function sum($a, $b) { return $a + $b; }
```

If you want to specify `mixed` type, you just say *mixed*:
```php
/**
 * @param mixed $options
 * @return mixed[] 
 */
function prepareSearch($options) { return [$options['q']]; }
```
If you used PHPDocs earlier in plain PHP, you probably got used, that *'mixed'* in PHPDocs means *'anything'*.  
But in KPHP, *mixed* is a type, so you can pass only arguments convertable to *mixed*.  

Since *mixed* is a mixture of any primitive types (numbers and strings for example), `int|string` is `mixed` indeed.
```php
/**
 * @param int|string $m
 */
function acceptsMixed($m) { }

acceptsMixed(5);           // ok
acceptsMixed(getMixed());  // ok, though not int|string
acceptsMixed([1,2,3]);     // ok, int[] is convertible to mixed, though not int|string
acceptsMixed(new User);    // error, instances are not compatible with mixed
```

If you don't specify *@param* for some arguments, they will be automatically inferred. (but don't do this)
```php
/**
 * @param int[] $a
 * @param ?bool $c
 */
function f($a, $b, $c) {}
// type of $b is automatically inferred, but it smells badly
```

PHPDocs for arrays help to deal with *Unknown* type. Imagine you are creating a function with *$options* array parameter, but this parameter is never passed yet:
```php
function renderMessageRow($row, $options = []) {
  if ($options['wrap_with_div']) {
    // ...
  }
  /* ... */
}     

renderMessageRow($row1);    // $options not passed
renderMessageRow($row2);    // $options again not passed, never
```
Without PHPDoc, KPHP can't infer type of *$options\[\*\]*, cause *$options* is constantly empty, type of its element is unknown. Specifying PHPDoc helps:
```php
/**
 * @param bool[] $options or mixed[]? you better know
 */
function renderMessageRow($row, $options = []) { /* ... */ }
```

PHPDocs can express any complex type supported by KPHP (see next chapter). They are applicable to arguments, return, class vars, local vars, globals, etc.  
But for simple primitives, you can use PHP 7 type hints instead.


## PHP 7 type hints

```warning
As for now, KPHP supports about PHP 7.2 language level.  
So, type hints can be used in functions — but not in class fields, and without union types.
```

```php
function sum(int $a, int $b): int { return $a + $b; }
```

Type hints are more lightweight and are preferred to be used for primitives. They work just as PHPDocs and lead to errors if passed incorrectly:
```php
function f(?string $b): int {
  return 3.4 * 2;  // compilation error
}

f(42);  // compilation error 
```

PHP itself checks type hints at runtime, but of course, if PHP code works, it might be not compiled:
```php
function f(int $x) {}

f(42);               // ok for PHP and KPHP
f("42");             // ok for PHP o_O (since string is numeric), error in KPHP: string instead of int
f($_GET['id']);      // ok for PHP if string is numeric, error in KPHP: mixed instead of int
```

You can't express *int[]* with type hint, as long as *int|false* (until PHP 8 at least). Use PHPDoc for such arguments, combining with type hints:
```php
/**
 * @param int|false $friend_id
 * @param bool[][] $bool_sub_options
 * @return mixed
 */
function f(int $my_id, $friend_id, float $coeff, $bool_sub_options) {}
``` 

Instances in type hints are supported. Be careful: *null* is OK for KPHP as instance, but not for PHP. 
```php
function f(CounterInterface $i): ?B { return null; }

f(new IntCounter);        // ok
f(new User);              // error unless User implements CounterInterface
f(null);                  // ok for KPHP, runtime error in PHP (?CounterInterface will fix PHP)
```

There is *array* type hint. For KPHP, it means **array of any type** (as it will be automatically inferred):
```php
function f1(array $a1) {}
function f2(array $a2) {}

f1([[1],[2],[3]]);
f1(["1", "2", "3"]);
f2([new User]);
```
*$a1* will be inferred `mixed[]`, *$a2* will be inferred `User[]`.

A suggestion is to use *@param* and *array* type hint simultaneously to control array types:
```php
/**
 * @param int[][] $a1  
 */
function f1(array $a1) {}

f1([[1],[2],[3]]);                              // ok
f1([['k1' => 1],['k2' => 2],['k3' => 3], []]);  // ok
f1(["1", "2", "3"]);                            // compilation error
``` 

If a type hint exists for an argument, and you want to add comment in PHPDoc, you should duplicate its type:
```php
/**
 * @param int $x Positive integer
 */
function f(int $x) {}
```

If you don't duplicate type, parsing will wait, treating comment as type:
```php
// error! Unknown class Positive
/**
 * @param $x Positive integer
 */
```

Type hints for lambdas also work:
```php
array_map(function(int $v): int { return $v + 1; }, $arr);     // ok
array_map(function(int $v): string { return $v + 1; }, $arr);  // compilation error
```

If you specify neither type hint nor *@param*, the argument will be automatically inferred. Again, it's a bad practice.

KPHP supports the following type hints: *int*, *bool*, *float*, *array*, *string*, *callable*, classes/interfaces, nullable type hints. 
*object* is not supported, as instances are strictly typed, you can't pass "any object". 

```warning
If *@var/@param/@return* is incorrect, or if type hint is incorrect, compilation will fail.  
This means, that probably lots of PHPDocs in your existing code should be fixed, as they won't match inferred.
```

## Compiler options to deny untyped code

There are two compiler options, 0 by default, but recommended being set to 1:

<aside class="nooffset">KPHP_REQUIRE_FUNCTIONS_TYPING = 0/1</aside>

If 0 (default):
* if *@param* or *type hint* exists, it is checked; else, the type of argument is auto inferred
* if *@return* or *return hint* exists, it is checked; else, the return of function is auto inferred

If 1 (recommended):
* if neither *@param* nor *type hint* exists, you get a compilation error: arguments typing is a must
* if neither *@return* nor *return hint* exists, **void** is assumed: return typing is a must unless void
* lambdas can still be left untyped (so, this applies to named functions only) 

<aside class="nooffset">KPHP_REQUIRE_CLASS_TYPING = 0/1</aside>

If 0 (default):
* if *@var* exists for a field, it is checked; else, the type of this field is auto inferred

If 1 (recommended):
* typing fields is a must — both static and instance fields
    * either you lock type with *@var*
    * or with a default value (more details in [instances](./instances.md#declare-field-type-with-a-default-value) article)  


```tip
## Recommended usage for declaring types
* declare types for all arguments: either *@param* or *type hint*
* declare return type for functions unless void: either *@return* or *return hint*
* declare types for class fields: either *@var* or a default value if option above is 1  
* optionally: declare types for local / global / static vars with *@var*

Recommended to assign 1 to both options mentioned above — and always use manual typing.
```

Now take a look, what types exist in KPHP, how they are produced, and how expressed in PHPDoc. 
