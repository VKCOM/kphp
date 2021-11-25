---
sort: 6
---

# Casting types

Here we talk about casting types — how to make type inferring more precise.
  

## Casting primitives with PHP operators 

PHP has language constructs `(int)` and so on. KPHP uses them for compile-time inferring and runtime casting.

A common intention is to cast strings to integers and vice versa:

```php
$a = isset($input['v']) ? (int)$input['v'] : 0;
$id = (int)"42";
$id_str = (string)$id;
```

Casts are pretty useful to narrow *mixed* to a presice primitive if we know exactly, what value it holds at runtime:
```php
function getUser($id) {   // returns mixed 
  if($id < 0) return "invalid id=$id";
  return ['id' => $id, ...];
}

$user = getUser(1);         // mixed
// but we are SURE $id is correct, it's array really
$user = (array)getUser(1);  // mixed[]
```

Without casts, you can't pass *mixed* to strictly typed functions:
```php
function getFriends(int $user_id) { /* ... */ }

$user = ['id' => 1, 'name' => "Alice"];   // mixed[]
getFriends($user['id']);                  // error! passing mixed to int
// but we are SURE that 'id' index is int at runtime
getFriends((int)$user['id']);             // ok
```

Casts can be used to drop false / null from primitives:
```php
$s_n = something() ? "hello" : null;   // string|null
// again, if we are SURE it's really not null
$s = (string)$s_n;                     // string
```

Using *(array)* cast is a bit tricky: `(array)mixed == mixed[]`, `(array)(T[]|false|null) = T[]`.
```php
function get($single) { return $single ? 1 : [1]; }   // returns mixed
$arr = (array)get(false);                             // mixed[]

$arr_or_null = something ? [1,2,3] : null;     // ?int[]
$arr = (array)$arr_or_null;                    // int[]
```


## Casting instances

When you get an object of a base class, you can do `instanceof` check for derived. It works exactly like in PHP.  
Inside `if ($x instanceof A)`, *$x* is assumed to be of class `A` **and should be read-only**:
```php
class Base { public $b; }
class Derived extends Base { public $d; }

function analyze(Base $o) {
  if ($o instanceof Derived) {
    $o->d;    // ok
  }
}
```

If you are whyever sure, that *$o* is *Derived* without *instanceof*, there is `instance_cast()` operator:
```php
/** @var Base[] $arr */
$d = instance_cast($arr[0], Derived::class);   // returns ?Derived (null if not of that class)
$d->d;    // ok if not null
```


## Casting instances to mixed

Instances are not compatible with *mixed*, but...

You can deeply convert an object to `mixed[]` with `instance_to_array()`.  
It is useful for logging/debugging, as instances are not allowed to be passed to *var_dump()*, etc:
```php
class Message {
  public $id;
  public $text;
  /** @var User */
  public $author;   // this field will be also converted (deep conversion is performed)
}
 
$message_as_array = instance_to_array($message);   // mixed[]
var_dump($message_as_array);
```

As a consequence, you can output an instance to JSON (KPHP has no native JSON serialization for instances yet):
```php
json_encode(instance_to_array($instance));
```

```note
Why `instance_to_array($obj)`, but not native PHP `(array)$obj`? Because PHP's *(array)$obj* is not suitable:
* remains nested instances — not converted to arrays, not compatible with *mixed[]*
* private and protected fields names are mangled with unprintable symbols

That's why *instance_to_array()* — natively in KPHP and polyfilled in PHP — to guarantee identical behavior.
```


## Smart casts

If you have already checked for *is_array($v)*, you don't need explicit *(array)$v*: KPHP will infer it automatically.

Example:
```php
function f_int(int $i) { /* ... */ }
 
function demo(?int $i_or_null) {
  if ($i_or_null !== null)
    f_int($i_or_null);
}
```

One more example:
```php
/**
 * @param tuple(int, A)|false $t
 */
function demo($t) {
  if ($t === false) 
    return;
  f_tuple($t);
}
```

One more example:
```php
/** @var (int|false|null)[] $arr */
foreach ($arr as $v) {
  if ($v === false || $v === null) 
    continue;
  f_int($v);
}
```

Even one more example!
```php
/** @var int|false $b */
if ($b) 
  f_int($b);   // KPHP understands, that false can't be here
```

Smart casts work **only for local vars**. They don't work for array indexing or property access.
```php
if ($a->int_or_false !== false) {
  // won't compile! can't pass 'int|false' to 'int'
  f_int($a->int_or_false);
}
```
Why? Because there can be another reference to *$a*, which can potentially change values bypassing control flow.

Similarly, smart casts don't work when variables are passed by reference.
```php
function changeVar(int &$ref) {}

/** @var ?int $a */
if ($a !== null) {
  // won't compile! can't pass '?int' to 'int'
  changeVar($a);
}
``` 

Also, smart casts don't work on re-assigning:
```php
function demo(?int $id) {
  if ($id === null) 
    $id = id();
  // error! $id here is still int|null
  f_int($id);
}
``` 

Don't confuse smart casts and lateinit pattern. Here is lateinit, not smart casts:
```php
class A {
  /** @var ?array */
  static private $cached_info = null;
 
  static function getInfo(): array {
    if (self::$cached_info === null) 
      self::$cached_info = loadInfo();

    return self::$cached_info;     // error! returning ?array instead of array
    // correct: (array)... or not_null(...)
  }
}
```


## not_null, not_false

These keywords tell KPHP, that you guarantee a variable to carry a non-empty value. Similar to *operator !!* in Kotlin.
```php
// correcting upper example
function getInfo(): array {
  if (self::$cached_info === null) 
    self::$cached_info = loadInfo();

  return not_null(self::$cached_info);
}
```

Use `not_null()` and `not_false()` when it is uninferrable or smart casts don't work, like above. It's much more convenient than *(casts)* and much more semantic. 


## Calling native functions does automatic casts

Say, you call *strlen()* function. It accepts a *string* argument. But if you call it with *integer* — it will be automatically cast. If you call it with *mixed* — the same. 
```php
function my_strlen(string $s) {}

my_strlen(4);              // error
strlen(4);                 // ok, automatically casted
my_strlen($user['name']);  // error
strlen($user['name']);     // ok, automatically casted 
```

```tip
No matter whether a native function accepts strings / ints / arrays — casts will be auto inserted.
```

**Why is it done in such a way?** Almost because when executed in plain PHP, standard library functions check input arguments, and if you pass something wrong — you'll see it while development, as opposed to functions written in PHP with auto inferred types. Moreover, such an approach requires less manual casts when using *mixed*, though it seems a bit incorrectly. Maybe, this behavior will be controlled by an option in the future.

When `declare(strict_types=1)` is used, implicit casting is disabled for that file. This applies to both native functions and functions annotated with `@kphp-infer cast`.
