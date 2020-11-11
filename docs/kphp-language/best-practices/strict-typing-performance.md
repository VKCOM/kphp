---
sort: 1
title: Strict typing is performance
---

# Strict typing increases performance

If you want performance — write types everywhere.


## PHP is not designed for typing, even type hints

Typical programming on PHP doesn't require a developer to think about types. 
You use numeric strings as integers, you mix different results into one hashmap. 

```php
// we know, that it's a number, but it's a numeric string at runtime
$user_id = $_GET['user_id'];
// we add string and integers
$friend_id = 100 + $user_id;
// we compare strings and integers
if ($user_id == $friend_id) { }
// we mix strings, integers and other types easily
$result = [
  'id' => $friend_id,
  'name' => "Alice",
  'deleted' => false,
  'friends' => [...]
];
```

PHP 7 introduces type hints, but...

```php
function lookupUser(int $user_id) { /* */ }

// it's a string! numeric, but still string
$user_id = $_GET['user_id'];
lookupUser($user_id);
// passed 'string' to 'int'! No warnings... PHP feels ok 
```


## KPHP urges you to think about types

The example above won't compile with KPHP:
```
Argument $user_id of function lookupUser(): expected int, got mixed
```

You'll need to fix it:
```php
$user_id = (int)$_GET['user_id'];
lookupUser($user_id);  // now ok
```

If you use PHP 7 type hints for function arguments / return value — all type correspondence must be satisfied. 
If you don't write type hints, use PHPDoc.


## KPHP has a 'mixed' type 

<div class="main-definition">
    mixed == (int OR float OR bool OR null OR string OR array of mixed)
</div>

If you have an existing PHP project, probably most of your code turns out to be *mixed*.


## mixed is slow, better use accurate types

KPHP `mixed` implementation is much more lightweight than PHP `ZVAL`, but still: the less you use it, the better performance is. 
*int* is better than *mixed*, *int[]* is much better than *mixed[]*.

Why mixed is slow? The first, memory: it can carry any value. The second, execution: 
```
$a + $b
```
* if *$a* and *$b* are `int`, it is one assembler instruction
* if *$a* and *$b* are `mixed`, it is executed like this: is *$a* string? is *$a* array? is *$a* int? ok, *$a* is int, and *$b*? is *$b* string? is *$b* array?..  

And this is true, conditions need to be handled, because *$a + $b* may be adding strings, may be adding arrays — we don't know this at compile-time.

For a practice demonstration, that *mixed* is slow, consider [this article](../../various-topics/walk-through-php-kphp-cpp.md) in the middle.


## mixed arguments are bad, they require runtime conversion

If a function accepts *mixed* and you pass *int* there, this *int* has to be converted to *mixed* at runtime.
```php
// suppose $arg was inferred as mixed
function f($arg) { /* ... */ }

f(42);    // this int needs to be wrapped to 'mixed' container and immediately destroyed after
``` 

If you pass *int[]* to *mixed* — every element needs to be converted, which can also slow down performance.    
```php
// suppose $arg was inferred as mixed
function f($arg) { /* ... */ }

f([1,2,3]);  // this leads to int[] -> mixed[] -> mixed runtime conversion; for big arrays takes time
```

```danger
Unnoticable in every particular place, using *mixed* everywhere is significant in total.  
Code with lots of *mixed* will run probably faster than PHP, but if you want to make it much faster, you should gradually rewrite it, making types be inferred more accurately. 
```


## Where do mixed come from?

Usually **mixed come from arrays with differently typed values**. 
```php
// user is mixed[]
$user = [
  'id' => 1,
  'name' => "Alice"
];
// $user[*] is mixed
$id   = $user['id'];    // mixed
$name = $user['name'];  // mixed
$first_char = $name[0]; // mixed

foreach ($user as $k => $v) {
  // $v is mixed, as $user is mixed[]
  // $k is mixed, as all keys of PHP arrays can be int|string
}
```

Superglobals like *$_GET, $_POST* and others are *mixed[]*. 

**Second**, *mixed* occurs when writing various types into one variable:
```php
$result = 'str';
if (some()) {
  $result = true; 
}
// $result infers as mixed 
```

**Third**, *mixed* is inferred if you pass different types to a function:
```php
function f($arg) { /* ... */ }

f(1);
f("1");
// $arg is mixed
```

```tip
Use type hints or *@param / @return* in function declaration to be sure of types.
```



## Casting mixed to accurate types

You can use standard PHP operators like `(int)` to cast:
```php
$m = getSomeMixed();
(int)$m;
(float)$m;
(string)$m;
(bool)$m;
(array)$m; // mixed[]
```

If *$m* is for example *[1,2,3]*, *(string)$m* will return `"Array"` and produce a warning, just as in PHP.

You'll need casting to assign *mixed* to accurate primitive types:
```php
function f(int $x) { } 

$m = getSomeMixed();
f($m);      // compilation error
f((int)$m); // ok
```

Casting a numeric string to *(int)* works fine too. Casting to *(bool)* returns boolval depending on the runtime type.

Consider an article about [type casting](../static-type-system/type-casting.md) as well.


## Avoid unexpected mixed, use PHPDoc

Always use `@param / @return` (or type hints) for functions to get compilation errors when type rules are broken.

While optional, using `@var` for local variables is a good practice. This prevents type from occasional change. 

For example, you had an array:
```php
// inferred as int[]
$admin_ids = [
  ID_KEVIN,
  ID_BOBBY,
]; 
```

Then later you write
```php
// suppose $config is mixed[]
// now $admin_ids infers as mixed[], not int[]!
$admin_ids[] = $config['cluster_owner_id'];
```

Type of *$admin_ids* is now *mixed[]* occasionally, but you didn't expect it, you probably will never notice.

**To ensure the type, you can specify it explicitly**:
```php
/** @var int[] $admin_ids */
$admin_ids = [ ... ];
```

Now you'll get a compilation error, that *$admin_ids* expected to be *int[]*, but inferred *mixed[]*. How to fix:
```php
// now ok, $admin_ids[] remains int[]
$admin_ids[] = (int)$config['cluster_owner_id'];
```

Consider an article about [using PHPDoc](../static-type-system/phpdoc-to-declare-types.md) as well.


## Use typed arrays, not just 'array'

When declaring *@param* for array argument, a suggested approach is to specify its type:
```php
/**
 * @param string[] $names
 */
function f(array $names) {}
```

If you write just *'array'*, an array of any type is supposed:
```php
function f1(array $arr1) {}
function f2(array $arr2) {}

f1([1,2,3]);
f1(['1','2','3']);

f2([1,2,3]);

// as a result,
// $arr1 will be inferred 'mixed[]'
// $arr2 will be inferred 'int[]'
```

So, *'array'* means *"it should definitely be an array, but an array of what type — doesn't matter"*.  
Even if you intentionally accept *mixed[]*, better specify it explicitly to emphasize.


## Instances are much better than associative arrays

Consider [an article about instances](../static-type-system/instances.md).

Why do instances consume much less memory and perform much faster?

* while all elements of hashtable have one type `array<T>`, every field of instances has **its own type**
* classes are codegenerated to **native C++ structures** with refcounter wrapper, not a hashtable
* that's why accessing an instance field is just getting **compile-time known offset** from memory, whereas accessing arrays leads to hash calculating, buckets lookup, etc
* instances consume **times less memory** than associative arrays
* instances are **reference types**, not copy-on-write
* you get **IDE autocompletion** and find usages

KPHP supports almost all PHP features: inheritance, interfaces, traits, etc. 

```warning
But instances have a strong limitation: it is not a subtype of *mixed*:
* you can not assign an instance and int to the same variable
* you can not pass an instance and a string to the same function argument
* you can not insert an instance and a number into the same array
* *var_dump()*, *json_encode()*, *serialize()* work only for mixed, not instances 
```

```php
// ok for PHP, not ok for KPHP
$result = [
  'mode' => $_GET['mode'],
  'hash' => substr(getCurrentAccessToken(), 0, -2),
  'user_info' => new User((int)$_GET['id']),
];
```

Even simple PHP-way approaches become incorrect: here you create an array with both *primitives* and *User*, this is incorrect from the type system view and leads to compilation errors.

How to solve this problem? 
* Create a wrapper class with 3 typed fields: *mixed $mode*, *string\|false $hash*, *User $user*
* Use *tuples*
* Use *shapes*
* Use *instance_to_array()*, recommended only for debug and logging purposes

[An article about the type system](../static-type-system/kphp-type-system.md) explains all these words.


## Configure KPHP to deny untyped code

Turn on some options, and KPHP will force you to declare *@param* / *type hint* / etc. everywhere.

This is explained in [an article about declaring types](../static-type-system/phpdoc-to-declare-types.md#compiler-options-to-deny-untyped-code).

If you install [KPHPStorm](../kphpstorm-ide-plugin) (plugin for IDE), it will show you when you miss type declarations.


```tip
Always treat KPHP like any other compiled language, and all limitations and errors will become obvious.
```

