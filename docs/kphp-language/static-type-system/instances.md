---
sort: 4
---

# Instances (objects)

Like in PHP, you declare classes and call `new ClassName(…args)`. They are **instances**, or objects (synonym).  
Unlike PHP, KPHP's instances have several limitations, that are similar to other compiled languages.


## KPHP compiles PHP classes to C++ structures

This is times more efficient than PHP `ZVAL` and consumes significantly less memory than associative arrays.  
KPHP classes behave identical to PHP: C++ wrapper `class_instance<T>` manages reference semantics.

```php
// PHP
namespace SomeNs;

class A {
    /** @var int */
    public $a = 20;
    /** @var string|false */
    private $name = false;

    // __construct() and other methods
}
```

Such class with two fields and no inheritance will be converted to C++ structure:
```cpp
// C++
struct C$SomeNs$A: public refcountable_php_classes<C$SomeNs$A> {
  int64_t $a{20L};
  Optional < string > $name{false};

  const char *get_class() const noexcept { return R"(A)"; }
  int get_hash() const noexcept { return 1128653198; }
};
```


## Fields of classes are typed

KPHP infers types of class fields separately. If you don't specify any type to a field, it will be auto inferred:
```php
class A {
  public $number = 0;   // inferred as 'mixed' due to string assignment below
}

$a = new A;
$a->number = "42";
```

Leaving fields untyped is strictly not recommended, use PHPDoc with *@var* to control types:
```php
class A {
  /** @var int */
  public $number = 0;
}

$a = new A;
$a->number = "42";       // compilation error
$a->number = (int)"42";  // ok
```

Once your code has been successfully compiled, you are ensured, that all types match.

In PHPDoc, a valid syntax is to use *@var* with text comments simultaneously:
```php
/**
 * You can write comment here
 * @var string|false and here
 */
public $str_val = false;
```

Using a single-line *@var* tag is a preferred code style convention:
```php
// this is ok
/** @var string|false user name or false if not loaded */

// this is not recommended, as pollutes code with useless empty lines
/**
 * @var string|false user name or false if not loaded
 */
```

```warning
For now, KPHP as about PHP 7.2 language level, that's why class fields type declarations are not parsed yet:  
`public int $a = 0;`  
Once supported, you'll be able to use this, and after PHP 8 support union types will be allowed.
```


## Compiler option to deny untyped classes

<aside class="nooffset">KPHP_REQUIRE_CLASS_TYPING = 0/1</aside>

By default, this option is 0. If you write *@var*, it would be checked; otherwise, field type would be auto inferred.

If set to 1 (recommended), typing fields is a must — both static and instance fields: 
* either you lock type with *@var*
* or with default value:  


## Declare field type with a default value

If compiler option *KPHP_REQUIRE_CLASS_TYPING = 1*, then **if no @var, default value defines field type**:
```php
class A {
  // no @var, but due to =0 it is declared as int
  public $a = 0;
}
 
$a = new A;
$a->a = 3;       // ok
$a->a = 'str';   // error
```

If you need *int OR something* — not *int* — you should manually write *@var*.

If the default value is *false*, *bool* will be assumed:
```php
class A {
  static public $inited = false;
}
A::$inited = true;   // ok
A::$inited = 'str';  // error
```
If you want *string|false* for example, declare *@var* explicitly.

As a result, instead of writing
```php
  /** @var string */
  public $name;
```
you can do
```php
  public $name = '';
```
It will be equal to KPHP, leaving fewer comments in code.  
(It's better to write *public string $name = ''*, but PHP 7.4 syntax is not supported yet)

If the default is an empty array — it will be like `array` in PHPDoc: an array of anything, as would be auto inferred. A good practice is to use *@var* to declare `T[]` anyway:
```php
class A {
  // ok, but better to declare @var T[] — what type is here
  public $arr = [];
}
 
class B {
  // and here is int[] — due to default value
  static public $ids = [336098765];
}
```

**This is just syntactic sugar** for fewer *@var* — and thus works only if *KPHP_REQUIRE_CLASS_TYPING = 1*.


## Limitations of instances

Main limitations are:
* instances **are not compatible with mixed**; you can't return an instance or a string from function; an array can't contain both instances and primitives
* you can only **access fields by a constant name**; `$a->$some_prop` is prohibited, calling a method by name can't be compiled also
* **exception inheritance** is not supported
* **no magic methods** like *__get()*, *__call()* and others
* **reflection** is not supported 
* **serialize(), var_dump(), json_encode() and others** can't be used with instances, as they accept *mixed* 

Tip: you can partly bypass the last limitation with `instance_to_array()` and [serialization](../howto-by-kphp/serialization-msgpack.md).


## Extends, implements — they work, just keep types in mind

Just as you can't mix instances and primitives, you can't mix instances, if they don't inherit a common base.

```php
interface I1 {}

class A1 implements I1 {}
class A2 implements I1 {}
class B1 {}

// suppose we have no @param for $arg, it's auto inferred
function f($arg) {}

f(new A1);    // here type inferring thinks: $arg is A1 
f(new A2);    // here type inferring thinks: $arg is A1|A2 => $arg is I1
f(new B1);    // here type inferring encounters an error, as B1 and I1 are incompatible
```

Same for any arbitrary nesting:
```php
function f() {
  return something
    ? tuple(1, [new A])
    : tuple(2, [new B]);    // error unless A and B have a common base
}
```


## instances can be null

For now, KPHP doesn't track nullability at compile-time, and instances can carry *null* values (like in PHP).  
To check whether *$o* is initialized, use `if ($o !== null)` or just `if ($o)`.
```php
function getCurrentUser(): ?User { /* ... */ }

$u = getCurrentUser();
if ($u)
  $u->logoutRedirect();
```

In PHPDoc and type hints, *ClassName* and *?ClassName* are treated the same. Use '?' to emphasize the ability of null. 

If you access a property of a *null* object, this object would be initialized in-place, triggering a runtime warning:
```php
/** @var User $u */
$u = null;
$u->id = 4;    // in-place initialization + runtime warning
```


## -> resolving: PHPDocs vs type inferring

When we write `...->f()`, the compiler should guess, what type is before an arrow. Method *f()* may exist in many classes, and KPHP should know the class to bind this call to a concrete function.

**This knowledge must before type inferring**. It is "apriori information", which you provide with PHPDoc and type declarations for IDE autocompletion.

Basically, such principle is mostly true: **if PhpStorm knows, that to suggest after ->, KPHP will guess also**. And vise versa: if IDE can't suggest, type annotation is missing somewhere and you should hint KPHP/IDE.

If a variable is created via *new* and used immediately — it is obvious without hints:
```php
$a = new A;
// ...
$a->f();   // compiler will bind this call to A::f()
```

If you pass an instance to a function — use *@param* or *type annotation*:
```php
/**
 * @param A $a
 */
function doSmth1($a) {
  $a->f();   // bind to A::f() thanks to @param
}

function doSmth2(A $a) {
  $a->f();   // bind to A::f() thanks to type annotation
}
```

Without *@param* or *type annotation*, neither IDE nor KPHP would guess that *A::f()* is being called.

To hint an array of instances, also PHPDoc is used:
```php
/**
 * @param $arr A[]
 */
function doSmth($arr) {
  $arr[0]->f();
  foreach ($arr as $a) {
    $a->f(); // guessing methods analyses foreach and other common use-cases 
  }
}
```

If a function returns an instance, use *@return* or type declaration:
```php
/**
 * @return A
 */
function getA() {  // or  :A
  /* ... */
  return $a;
}
 
getA()->f();  // bind to A::f(), thanks to @return
``` 

Same for arrays:
```php
/**
 * @return A[]|false
 */
function getMaybe() {
  if(0) return false;
  return [new A('1')];
}
 
$arr = getMaybe();
echo $arr && $arr[0] ? $arr[0]->name : "no";
```

Same for tuples and any other types, that contain instances inside:
```php
/**
 * @return tuple(int, A)
 */
function getCountAndObject() { /* ... */ }

$a = getCountAndObject()[1];
$a->f();  // bind to A::f(), thanks to @return; IDE with KPHPStorm would also suggest method f()
// without @return, neither KPHP nor IDE would guess 
``` 

When assigning something non-trivial, sometimes *@var* hint is needed explicitly:
```php
/** @var $a A */
$a = condition ? [[new A('d')]][0][0] : null;
```

The same, an example with an initially empty array:
```php
// need to hint, as from initial assignment you can't suppose anything
/** @var A[] */
$arr = [];
/* ... */ $arr[] = new A;
 
$arr[0]->f(); // bind to A::f(), thanks to explicit @var near assignment
```

When a field of a class contains another instance, it must be also annotated with *@var*:
```php
class A {
  /** @var User */
  public $user;
  /** @var A[] */
  public $a;
}

$a->user->name;            // User::name, thanks to @var
$a->a[0]->a[1]->user->id;  // also resolved, thanks to @var 
```


```note
Concluding, PHPDoc is needed for **early binding** methods and fields. It is done **before building a call graph** and thus **before type inferring**.  
You can try to cheat on the compiler, having written incorrect PHPDoc. In this case, methods would be bound incorrectly, but type inferring afterward would detect it and produce an error.  

So, declared PHPDoc types for instances is apriori information, for early binding.  
Then — type inferring, based on the call graph.  
Then — type checking, that inferred types match the declared.
```


## Using instances incorrectly will not compile, it's on purpose

You will definitely feel discomfort, that you can't mix instances with primitives, that you can't mix instances unless they have a common ancestor. 
 
The border between the untyped PHP way and typed KPHP way is very painful. Lots of your existing code won't compile, you'll need to rewrite it correctly from the point of the type system. But once isolated parts of code become fully typed, they are found to be much cleaner, expressive, and faster than before.
  
And you practically never need *mixed*, it's left almost to interop with old, untyped areas. 


```tip
All in all, when you use instances, declare all types and use them in a compiled way, not in a PHP way.  
As a result, your code can be made fast, as if it was written directly in C++.
```
