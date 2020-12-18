---
sort: 7
---

# Exceptions 

KPHP supports most exception-related features, but doesn't support `finally` keyword. 
```php
// Exception is a builtin class that can be extended
function loadUser(int $user_id): User {
  if ($user_id <= 0)
    throw new Exception("Invalid user_id = $user_id");
  /* ... */
}

try {
  $user = loadUser($id);
} catch (Exception $ex) {
  echo $ex->getMessage();
}
```

Traces are not identical to PHP, but more or less readable.

## How exceptions are implemented

Internally, KPHP doesn't translate this code to C++ exceptions for several reasons.  
Instead, KPHP builds exception spreading graphs and detects, what functions and statements can throw. When an exception is thrown, an internal global variable is set. Potentially throwing statements are wrapped with a macro, that checks this global and returns if true. This approach in fact works faster than C++ exceptions.

If an exception has been thrown inside an async call, it is suppressed and would be re-thrown when waiting for the async result. This is described in [coroutines](../best-practices/async-programming-forks.md).

To be sure, that a function never throws an exception, use `@kphp-should-not-throw` annotation above it.   


## Prefer error codes to exceptions

You are sure, that external code handles an error — otherwise, it would not compile.

A well-known pattern is to return `tuple(error, result)`. An error can be int/string/some class. Typically checked with `if(!$error)` — you return 0 for int, empty for string, null for an object or nullable type, etc.
```php
/** @return tuple(int, string) [error_code, server_name] */ 
function chooseUploadServer() { /* ... */ }

/** @return tuple(?string, ?User) */
function loadUser() { /* ... */ }

[$err, $user] = loadUser();
if ($err) {
  // handle
  return;
}
$user->id;
```
