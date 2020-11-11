---
sort: 1
---

# Type inferring

KPHP urges you to think about types. It's the reason you can write code that executes much faster than PHP.  
You can omit types in your code — KPHP will **infer** them as precise as possible. With some limitations.


## PHP allows you to mix everything, KPHP doesn't

In PHP, every variable can hold any value — `ZVAL` in internal PHP terms. You just dynamically handle it:
```php
// in PHP, you can pass anything to a function (but it won't compile with KPHP)
function printMe($something) {
  if (is_object($something) && $something->id) 
    echo "object with id {$something->id}";
  else if (is_array($something) && isset($something['name'])) 
    echo "array with name index";
}

// in PHP, you can mix anything in an array (but in won't compile with KPHP) 
$data = [
  'id' => 3,
  'details' => [ /* nested array */ ],
  'main_friend' => new User(),
];
```

```warning
In KPHP, every variable must be of a valid type. If you mix types incorrectly, you'll get a compilation error. 
```


## What is type inferring

KPHP converts PHP to C++, and thus all variables must be declared with C++ types.  
KPHP infers types of all variables — how to declare them in C++ source code to make it effective.

```php
// PHP
$a = 12;
echo $a;
```
Having this PHP code, KPHP will understand, that *$a* is always a whole number, and declare it as `int` in C++.
```cpp
// C++
int a;
a = 12;
print (strval (a));
```

The same is done for functions arguments — KPHP tracks all available invocations:

```php
// PHP
function f($arg) { /* ... */ }

f("12");
f((string)$_GET['id']);
f(['1', '2', '3'][0]);
```
Here *f()* is called only with a *string* argument, so KPHP infers `string`:
```cpp
// C++
void f(string arg) { /* ... */ }
```

KPHP tracks all assignments. If a variable is an array — KPHP analyzes all types inserted into it:
```php
// PHP
$arr = [1, 2];
$arr[] = 3;
array_push($arr, 4, (int)$_GET['id']);
```

Here KPHP sees, that only *int* values are inserted to *$arr*, so infers it as `array<int>`.  
(`array<T>` and `T[]` are the same)
```cpp
// C++
array<int> arr;
arr = const_array_usoim23v8023;
arr.append(3);
array_push(arr, 4, intval(_GET.get_value(const_string_us3vpai3)));
```

KPHP also analyzes all function returns: 
```php
// PHP
function getName() {
  if (something()) {
    return null;  
  }
  return "str";
}
```
*getName()* is inferred to return `string|null`.  
(`T|null` and `?T` are the same)
```cpp
// C++
Optional<string> getName() { /* ... */ }
```

```note
C++ code is more complicated actually.  
For instance, PHP variable *$a* in C++ code is named *v$a*, not *a*; *int* is in fact *int64_t*; *const&* for read-only, etc.   
Let's ignore these moments to make examples clear.
```


## KPHP has a 'mixed' type as a mixture of primitives

<div class="main-definition">
    mixed == (int OR float OR bool OR null OR string OR array of mixed)
</div>

```php
// PHP
$a = 12;
if (something()) 
  $a = "12";
echo $a;
```

As you assign strings and numbers to one variable, KPHP will infer `mixed` type:
```cpp
// C++
mixed a;
a = 12;
if (boolval(something())) 
  a = const_string_usdio32n439;
print (strval (a));
```

If you declare an array with various primitives, this would be an array of *mixed*:
```php
$user = [          // inferred as array<mixed>, as values are string/numbers/mixed[] together
  'id' => 1,
  'name' => "Alice",
  'details' => [ /* nested array of primitives */ ]
];
```

It's very likely to get *mixed* occasionally:
```php
function f1($arg1) { /* ... */ }

f1(42);
f1("42");
```
KPHP analyzes, that *f1()* is called with various argument types, and infers *$arg1* as `mixed`.

```php
class A { public $field; }

$a1 = new A;
$a1->field = 42;

$a2 = new A;
$a2->field = getName();
```
Here we assign `int` and `?string` to *A::$field*, so it is inferred as `mixed` again.

```danger
As you see, it's very likely to emerge *mixed*. It is such a common PHP practice to mix primitives, that without this type you have zero chances to make your code compatible with KPHP.  
But *mixed* consumes memory and works much slower than native primitives. If you want to write fast code, think about it, because [strict typing is performance](../best-practices/strict-typing-performance.md).
```


## PHP has no function overload, KPHP too

```php
f(42);
f('str');
```
KPHP will infer *f(mixed)*, and every function invocation **will lead to converting an argument to *mixed* at runtime**.

One PHP function == one C++ function (except for template functions, discussed later).

Why can't KPHP generate two functions — *f(int)* and *f(string)* — to avoid runtime conversions? 
The problem is deeper than it seems and is discussed [here](../../various-topics/why-no-overload.md).


## Splitting variables with the same name

KPHP analyzes control flow and splits one variable if it is used in independent contexts:
```php
// PHP
$a = 4;
echo $a;
$a = 'str';
echo $a;
```
```cpp
// C++
int a_v0;
string a_v1;
a_v0 = 4;
print (strval (a_v0));
a_v1 = const_string_us199e52a2;
print (a_v1);
```

Another example. Here we also have 2 implicit vars: *string* as argument and *int* in the body:
```php
// PHP
function demo($arg) {
  $arg = (int)$arg;
  echo ++$arg;
}
demo('99');
```
```cpp
// C++
void demo(string arg_v1) {
  int arg_v0;
  arg_v0 = intval (arg_v1);
  print (strval (++arg_v0));
}
``` 


## Examples of type inferring

To make this page shorter, various demos "PHP code — what types inferred" are listed on a [separate page](../../various-topics/type-inferring-examples.md).


```tip
You can omit types while development, KPHP will infer them automatically. But even omitting, you should always think about them, because strict typing is performance.  
Use PHPDocs and type hints to control types manually — the next article is dedicated to it.
```
