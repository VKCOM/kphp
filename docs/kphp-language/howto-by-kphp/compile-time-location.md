---
sort: 7
---

# Compile-time location 

KPHP allows you to get an exact location, where a cocrete function was called (a previous function at call stack, file name and line number). As if `debug_backtrance()` was called — but at compile time.

For example, it's possible to implement `log_info($message)`, so that this code
```php
log_info("start calculation");
$arr = [1,2,3,4,5];
foreach ($arr as $v) {
  if ($v > 3)
    log_info("big v=$v");
}
log_info("end calculation");
```

would print:
```text
start calculation (in demo5 at dev.php:55)
big v=4 (in demo5 at dev.php:59)
big v=5 (in demo5 at dev.php:59)
end calculation (in demo5 at dev.php:61)
```

Notice, that `log_info()` prints out a parent (caller) location every time. It differs from time to time, it's unique for every call place. 

Here's the implementation:
```php
function log_info(string $message, ?CompileTimeLocation $loc = null) {
  $loc = CompileTimeLocation::calculate($loc);
  echo "$message (in {$loc->function} at {$loc->file}:{$loc->line})\n";
}
```

In a PHP polyfill, `CompileTimeLocation::calculate()` calls `debug_backtrace()` at runtime, but KPHP just appends an implicit argument to every call:

```php
// a regular call
log_info($message);
// is implicitly changed to
log_info($message, new CompileTimeLocation(__FILE__, __METHOD__, __LINE__));
```

A little notice: like in case of exceptions, a file name in KPHP is relative to a project root (whereas it's absolute in PHP). It makes resulting C++ code not to depend from a tmp path on a build agent, to leave it equal and reusable across machines codegeneration. 

By the way, you can pass `$loc` somewhere further. For example, here's how you can make wrappers around some logger, proxying a call location:
```php
function my_log(CompileTimeLocation $loc = null) {
  $loc = CompileTimeLocation::calculate($loc);
  ...
  generic_log($loc);
}
```
