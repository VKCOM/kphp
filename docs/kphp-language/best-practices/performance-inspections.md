---
sort: 6
title: "Performance inspections"
---

# Compile-time performance inspections

Can you explain, what is bad in this code?

```php
function printArray(array $arr) {
  foreach ($arr as $v)
    echo $v;
}

function badFunction(int $one, int $two) {
  $numbers = [$one, $two, 3];
  printArray($numbers);
}

printArray((array)$_GET['ids']);
```

It's hard to say. Here is the answer:

1. `printArray()` has an argument declared as `array`, actually meaning "an array of something, how it would automatically infer", and it was inferred as `array<mixed>` due to `$_GET['ids']`
2. but `$numbers` in `badFunction()` in just `array<int>`
3. we call `printArray($numbers)`, passing `int[]` to `mixed[]`; it's okay from type system's point of view
4. **hence, we get a runtime conversion from `int[]` into `mixed[]`**

All in all, a harmless line `printArray($numbers)` effectively means: create `$tmp_array : array<mixed>`, copy all elements one by one, wrapping every *int* into *mixed*, call `printArray($tmp_array)`, and then clear `$tmp_array`, calling destructors for all *mixed* wrappers. If an array is big, if affects performance, though we don't expect it at all.


## @kphp-warn-performance implicit-array-cast

Let's add such annotation over the function:

```php
/**
 * @kphp-warn-performance implicit-array-cast
 */
function badFunction(int $one, int $two) { ... }
```

After compilation, we see an error:

```text
//  12:  printArray($numbers);
variable $numbers is implicitly converted from array< int > to array< mixed >
Performance inspection 'implicit-array-cast' enabled by: badFunction
```

KPHP now doesn't allow such call, since it's bad, regardless of validness.

Another example, a part of a real project:

```php
function logMe(User $user, UserConnection $conn) {
  $log_info = [
    'id' => 3,
    'friend_ids' => $user->friend_ids,
    'connections' => $conn->dependencies,
  ];
   ...
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function loadInfo() {
  $user = RPC::loadUser(...);
  $connection = RPC::loadUserConnections(...);

  logMe($user, $connection);
  return tuple($user, $connection);
}
```

It's true that we load all data in a typed way, but we call a legacy logger which pushes typed data (`int[]` friend ids) into a single `mixed` hashmap, again leading to runtime conversions.

```text
//   7:  $log_info = [
variable User::$friend_ids is implicitly converted from array< int > to mixed
Performance inspection 'implicit-array-cast' enabled by: loadInfo -> logMe

//   7:  $log_info = [
variable UserConnection::$dependencies is implicitly converted from array< string > to mixed
Performance inspection 'implicit-array-cast' enabled by: loadInfo -> logMe
```

As you see, `@kphp-warn-performance` **analyzes all the call stack in depth**. If you write such an annotation at a higher-level function, it will check everything reachable from it.


## @kphp-warn-performance array-merge-into

That's not all! There are three more patterns this annotation can find.

```php
/**
 * @kphp-warn-performance array-merge-into
 */
function doSome(array $a) {
  $a = array_merge($a, [1,2,3]);
}
```

Will lead to

```text
//   7:  $a = array_merge($a, [1,2,3]);
expression $a = array_merge($a, <...>) can be replaced with array_merge_into($a, <...>)
Performance inspection 'array-merge-into' enabled by: doSome
```

What is it about? `array_merge($a1, $a2)` creates an array from these two and returns it. But when this array is immediately assigned into a variable just passed as an argument, it happens that *refcount* is increased at a call, an array is fully copied, then merged, then assigned, and a copy is erased. A solution is to use a built-in KPHP function `array_merge_into()`, which takes an array by reference, it's not *O(n^2)* in memory. As you see, this annotation effectively finds such occasions.


## @kphp-warn-performance array-reserve

A code snippet that you probably met lots of times in real life:

```php
function multiplyByTwo(array $numbers): array {
  $result = [];
  foreach ($numbers as $n)
    $result[] = $n * 2;
  return $result;
}
```

Again, it seems lightweight. But after annotating, we see:

```text
//   9:    $result[] = $n * 2;
variable $result can be reserved with array_reserve or array_reserve_from out of loop
Performance inspection 'array-reserve' enabled by: multiplyByTwo
```

What is it about? We append data into `$result` in a loop, which leads to memory reallocations. But we know in advance, that we'll insert N elements, and it's better to preallocate memory to avoid reallocations on inserting. 


## @kphp-warn-performance constant-execution-in-loop

And the last example.

```php
function outputSquares(array $numbers, array $options) {
  foreach ($numbers as $n)
    echo $options['prefix'] . $n . "\n";
}
```

Here we get:

```
//   8:    echo $options['prefix'] . $n . "\n ";
array element $options['prefix'] can be saved in a separate variable out of loop
Performance inspection 'constant-execution-in-loop' enabled by: outputSquares
```

KPHP notices that `$options['prefix']` is accessed every time within a loop, which leads to hashtable lookup. Since `$options` is guaranteed to be left unchanged, it's better to read `$options['prefix']` in advance.

Lots of constant expressions are analyzed. For example, concatenations of variables which don't depend from loop variables. KPHP will even offer to store `$z[floor(sin($x))]` outside if it's correct.


## @kphp-warn-performance all

Turns on all inspections mentioned above. 

Also, you can enable a specified list, separated by spaces:

```php
/**
 * @kphp-warn-performance implicit-array-cast array-reserve
 */
```


## Turning off inspections with !

```php
/**
 * @kphp-warn-performance all !implicit-array-cast
 */
```

Not hard to guess that such line activates all but one.

It's a way to suppress inspections at a function level. As you remember, an annotation works for the whole stack depth. And if KPHP is wrong somewhere deep inside or you just don't have time to fix it right now — a warning can be suppressed at the exact place. An example:

```php
/**
 * @kphp-warn-performance all !implicit-array-cast
 */
function legacyLog(...) {
  // here we allow merging T[] into mixed, as above
}

function loadInfo(...) {
  RPC::call(...);
  legacyLog(...);
}

/**
 * @kphp-warn-performance all
 */
function apiCall() {
  $info = loadInfo();
  return apiOutput($info);
}
```

A high level function `apiCall()` will propagate all the reachable callstack except *implicit-array-cast* in `legacyLog()`.


## @kphp-analyze-performance …

Another annotation. It accepts exactly the same arguments as `@kphp-warn-performance`.  
But opposed to *warn*, it does not trigger compilation errors: it outputs a json report instead.

```text
{KPHP_DEST_DIR}/performance_issues.json
```

`@kphp-warn-performance` annotation outputs only the first 3 errors intentionally. And here you can look through all the compile-time performance report even if it contains thousand items. This annotation is similar to `@kphp-profile`: you write it, compile, see a report, but don't commit. As opposed to *warn*-annotation, which should definitely be commited.

KPHPStorm knowns these two annotations, it suggests arguments and validates them.


## Conslusion: how to use compile-time performance inspections

KPHP is able to detect unoptimal code patterns, but there are usually lots of them in real code. It's impossible to fix them all, that's why KPHP doesn't fire them by default. Instead, you write annotations above "good" functions.

For example, you want to optimize a function. You annotate it with `@kphp-analyze-performance all` and get a json report, it probably contains lots of lines. Little by little, you fix them (not all at once, just iteratively). And when all of them are fixed — `@kphp-warn-performance all` and commmit. After, if somebody spoils the code even deeply in the call stack, compilation will fail. In particular places you can suppress inspections by `!`.
