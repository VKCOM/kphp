---
sort: 1
---

# Call a function by name

An ability to call a function/method by name does not exist in compiled languages. 
But it exists in plain PHP and is essential in various routing/ORMs and so on.

KPHP **does not support** calling by a dynamic name, just like any other compiled language.   
How to achieve the same result?


## How is it done in plain PHP?

In PHP, you just call any function by name / any method by name / create a class by name:

```php
$f_name = "...";
$f_name();
$f_name(1, "with", ["arguments"]);    
// and methods
$method_name = "...";
A::$method_name();
call_user_func(['A', $method_name]);
// and classes
$class_name = "...";
new $class_name("constructor", "parameters");  
```

```warning
This is prohibited in KPHP. All calls, all property access — everything must be statically resolved. 
```


## How to achieve this in KPHP?

Think of KPHP as of any other compiled language. 
For dynamic variables, nothing better than *switch-case* actually exists.

And this is OK, this is not a drawback. 
Static call graph makes your code predictable and understandable, you always see all available methods reached out from here.

```php
$func_name = "...";
switch ($func_name) {
case "authenticate":
  authenticate();
  break;
case "logout":
  logout();
  break; 
}
```


## Compile-time known strings and generics

If `$class_name` is compile-time known, you are able to create it via `new $class_name` or call like `$class_name::method()`:

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

It works **only for compile-time known variables**, in [generic functions](../static-type-system/generic-functions.md). It can work, because everything is still statically resolved.

Probably, when you are searching for a way to "create any class" or "call any function", that "any" comes from an input. Hence, generics will not fit your needs.


## If so, how do I perform request routing?

Let's say you want **your-site.com?page=profile&action=edit** to call `\UrlHandler\Profile::edit()`. 
You have various classes in *\UrlHandler* namespace for *&page* get parameter and every class has a method for *&action*.

First of all, **even in plain PHP, it's a bad idea to do like this**:

```php
// bad: primitive url mapping
$class_name = "\\UrlHandler\\" . ucfirst($_GET['page']);
$method_name = $_GET['action'];
$class_name::$method_name();
```

Why is it a bad pattern? Take these points under consideration:
* you don't have a full list of all available URLs
* if an action has been renamed, to make old links work you should handle it somewhere
* when an URL requires the user to be logged, you check this in each handler
* when an URL is available only for admins/VPN, you check this in each handler
* when an URL distinguishes GET/POST/etc, you handle this in each handler

```warning
"You handle this in each handler" using PHP code is not a good practice — not just because it leads to code duplication — 
but because you spread the area of responsibility between routing and business logic.
```

```tip
Declarative descriptions are much acceptable than imperative handling.
```
 
Having all this, you probably already use a declarative manner:

* Either you write something like this:

```php
Route::get('/profile/edit', ['logged' => true], 'Profile::edit');  
```

* Or you use annotations, something like this:

```php
class Profile {
  /**
   * @url-handler logged:true method:get
   */
  static public function edit() { /* ... */  }
}
```

* Or you have a schema, JSON/PHP/doesn't matter:

```json
[
  { "url": "/profile_edit", "logged": true, "method": "get", "handler": "Profile::edit" }
]
```

All these approaches **are similar** for the following reasons:
1. you use declarative manner, not imperative logic for common handling parts (*logged*, etc)
2. you have a full list of what can be called
3. **calling by name is OK here** — its safeness comes from your source code, it happens only on correct input

Because of (2), you can write a script that generates a *switch-case* code — and use it instead of (3).

```tip
## This article tried to address the following:
* either you don't need calling by name at all,
* or you have 2-3 cases and write *switch-case* manually,
* or you should have a declarative source, from which you can codegenerate *switch-case*; if you don't, you are doing something wrong anyway
``` 
