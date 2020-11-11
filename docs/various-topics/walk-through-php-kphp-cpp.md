---
noprevnext: true
notopnavigation: true
---

# Walk-through: benchmarking PHP vs KPHP vs C++

* take a PHP code and measure its execution time
* compile with KPHP and compare
* rewrite in plain C++ and compare
* tweak PHP code to make KPHP run faster
* add some abstractions and see, how it affects performance on PHP/KPHP

```note
We'll do all our experiments on a quicksort algorithm [from here](https://en.wikibooks.org/wiki/Algorithm_Implementation/Sorting/Quicksort#PHP) (the last one).  
You can oppose, that it has no real-life relation. Well, but no abstract piece of code has a real-life relation.  
The main idea to assimilate will be seen in the end, when we add abstractions to this code.
```


## PHP code: quicksort algorithm

```php
function quickSort(array &$a, int $start = 0, int $last = null) {
  $wall = $start;
  $last = is_null($last) ? count($a) - 1 : $last;
  if ($last - $start < 1)
    return;

  switchValues($a, (int) (($start + $last) / 2), $last);
  for ($i = $start; $i < $last; $i++)
    if ($a[$i] < $a[$last]) {
      switchValues($a, $i, $wall);
      $wall++;
    }

  switchValues($a, $wall, $last);
  quickSort($a, $start, $wall - 1);
  quickSort($a, $wall + 1, $last);
}

function switchValues(array &$a, int $i1, int $i2) {
  if ($i1 !== $i2) {
    $temp = $a[$i1];
    $a[$i1] = $a[$i2];
    $a[$i2] = $temp;
  }
}

function generateArr(int $size): array {
  $arr = [];
  for ($i = 0; $i < $size; $i++) {
    $arr[] = (int) (rand() / (1000000000 / $size));
  }
  return $arr;
}

$size = 1000000;
$arr = generateArr($size);
$start = microtime(true);
quickSort($arr);

$duration = round((microtime(true) - $start) * 1000 * 1000) / 1000;
echo "Sorted $size elements in {$duration}ms\n";
```


## Compare PHP vs KPHP

Let's launch it several times and take the average: 

<p class="pay-attention">
  <b>PHP 7.4</b>: ~2100 ms<br>  
  <b>KPHP</b>: ~480 ms <i>(~4x faster)</i>
</p>



## Okay, can we do better in C++?

Let's do the same in C++ and compile with *g++* with *-Os* optimization level (the same as used for KPHP):

```cpp
#include <iostream>
#include <cstdlib>
#include <chrono>
using std::chrono::steady_clock;

void switchValues(int64_t *, int64_t, int64_t);

void quickSort(int64_t *a, int64_t size, int64_t start = 0, int64_t last = -1) {
  int64_t wall = start;
  last = last == -1 ? size - 1 : last;
  if (last - start < 1)
    return;

  switchValues(a, (start+last)/2, last);
  for (int i = start; i < last; ++i)
    if (a[i] < a[last]) { 
      switchValues(a, i, wall); 
      wall++; 
    }

  switchValues(a, wall, last);
  quickSort(a, size, start, wall-1);
  quickSort(a, size, wall+1, last);
}

void switchValues(int64_t *a, int64_t i1, int64_t i2) {
  if (i1 != i2) 
    std::swap(a[i1], a[i2]);
}

int main() {
  int64_t size = 1000000;
  int64_t *a = new int64_t[size];
  for (int64_t i = 0; i < size; ++i)
    a[i] = std::rand();

  steady_clock::time_point begin = steady_clock::now();
  quickSort(a, size);
  steady_clock::time_point end = steady_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count() << " ms\n";
  return 0;
}
```


## Compare PHP vs KPHP vs C++

<p class="pay-attention">
  <b>PHP 7.4</b>: ~2100 ms<br>  
  <b>KPHP</b>: ~480 ms<br>
  <b>C++</b>: ~220 ms
</p>

Well, C++ is almost 2x faster than KPHP-compiled. Why, and can we do better in KPHP?


## Why is C++ faster than KPHP

Primarily it's because of the **array**. KPHP's *array* behaves like PHP, but in PHP it can be either a hashtable or a vector (with integer indexes 0, 1, 2, … — it's our case).
  
This means, that reading `$a[$i]` for numeric *$i* in KPHP works like this:
* *$a* is a wrapper with refcounter pointing to actual data, get it
* if *$a* is a hashtable, that perform bucket-accessing logic
* if *$a* is a vector, then check dimensions *($i>=0 && $i<size)*, and if satisfied, do a linear memory access

So, **reason number one**:
1. In C++, we did just `int* a` and `a[i]` — we did no checks for dimensions, and we know it's a vector
2. But in PHP, `array $a` and `$a[$i]` leads to *hashtable/vector* checks and dimensions checks

Next, look at this code PHP / C++:
```
// PHP                 // C++
$temp = $a[$i1];       std::swap(a[i1], a[i2]);
$a[$i1] = $a[$i2];
$a[$i2] = $temp;
``` 

In C++, we don't do any checks and just swap two 8-bytes memory pieces: we know, that they don't intersect in memory, that we won't access corrupted memory, etc.  
In KPHP, `$a[$i] = …` may lead to memory reallocation. For instance, `$a[100500] = 0` will convert a vector array to an associative array with 100500 index present, if its length was smaller. That's why `$a[$i1] = $a[$i2]` can potentially invoke memory reallocation, and in order to maintain pointers if it occurs, KPHP implicitly inserts an extra variable with explicit copying before assigning. Again, in C++ we just didn't take care of this, as while writing, we knew, that all dimensions are satisfied.

So, **reason number two**:
1. In C++, we did just `swap(a[i1], a[i2])` — direct swapping memory pieces
2. But in PHP, lines with *$temp* var not only lead to extra checks while accessing array by index, but also insert a special wrapper in case memory reallocation occurs (which actually doesn't occur here of course) 


## Can we do better in KPHP?

Let's start. We remember, that the bottleneck here is `$a[$index]` access.

First, take a look at this:
```php
  for ($i = $start; $i < $last; $i++)
    if ($a[$i] < $a[$last]) { /* ... */ }
```
We see, that `$a[$last]` is queried for every *$i*, but we are sure, that in our case *$last*-th element will remain the same. Let's extract it as a separate variable:
```php
  $a_last = $a[$last];
  for ($i = $start; $i < $last; $i++)
    if ($a[$i] < $a_last) { /* ... */ }
```

This really speeds up PHP/KPHP, but the same change in C++ does almost nothing because of cheapness.

Second, let's use `array_swap_int_keys()` KPHP built-in function:
```php
function switchValues(array &$a, int $i1, int $i2) {
  array_swap_int_keys($a, $i1, $i2);
}
```

This function performs checks only once and performs in-memory swapping for POD types, avoiding checking on every index access and extra code surrounding potential reallocations, as mentioned.

To make it work on PHP, either write a polyfill manually:
```php
#ifndef KPHP
function array_swap_int_keys(array &$a, int $idx1, int $idx2): void {
  if ($idx1 != $idx2 && isset($a[$idx1]) && isset($a[$idx2])) {
    $tmp = $a[$idx1];
    $a[$idx1] = $a[$idx2];
    $a[$idx2] = $tmp;
  }
}
#endif
```

Or — better — use [existing polyfills](../kphp-language/php-extensions/php-polyfills.md) convering all KPHP built-ins.

Third, look at array generation:
```php
  for ($i = 0; $i < $size; $i++) 
    $arr[] = (int) (rand() / (1000000000 / $size));
```
We know exactly, that *$size* elements would be inserted, and *$arr* would be a vector. It's a good idea to pre-reserve memory for *$size*-vector of integers, to avoid reallocations while inserting:
```php
  $arr = [];
  array_reserve($arr, $size, 0, true);
  /* ... */
```


## Compare PHP vs KPHP vs C++ — number 2

Let's see what we have achieved by optimizations above:

<p class="pay-attention">
  <b>PHP 7.4</b>: ~2650 ms <i>(was 2100)</i><br>  
  <b>KPHP</b>: ~270 ms <i>(was 480)</i><br>
  <b>C++</b>: ~220 ms
</p>

KPHP is now almost identical to C++! Wow.  
But... PHP became slower?? Why?

```tip
In practice, if PHP becomes a bit slower, don't pay attention to it: PHP for development, KPHP for production.
```


## Fix that PHP became slower

Remember, that we started using `array_swap_int_keys()` from `switchValues()`?

```php
function switchValues(array &$a, int $i1, int $i2) {
  array_swap_int_keys($a, $i1, $i2);
}
```

This function is invoked many-many times, and in PHP function calls are quite expensive. If we directly call *array_swap_int_keys()* without calling *switchValues()*, it will speed up PHP:
```php
    if ($a[$i] < $a_last) {
      array_swap_int_keys($a, $i, $wall);  // not switchValues()
      $wall++;
    }
``` 

In KPHP, this doesn't matter, as a simple function *switchValues()* is just inlined and has no overhead.


<hr>

## Compare PHP vs KPHP vs C++ — final results

<p class="pay-attention">
  <b>PHP 7.4</b>: ~1950 ms<br>  
  <b>KPHP</b>: ~270 ms <i>(~7x faster)</i><br>
  <b>C++</b>: ~220 ms
</p>

<hr>


## Can KPHP occasionally work slower?

KPHP works fast here, as it infers *$arr* to be an array of *int*. Let's say, `generateArr()` will append user input (*$argv*) to random numbers:
```php
function generateArr(int $size): array {
  /* ... */
  if (isset($argv[1])) 
    $arr[] = $argv[1];   // suppose it's a numeric string
}
```

Then, compile, run, and...

<p class="pay-attention">
  <b>KPHP</b>: ~940 ms <i>(was 270)</i>
</p>

What happened? Types have spoilt. *$arr* was *int[]*, but now it's *mixed[]*, because *$argv\[$i]* is *mixed*. As a result, *quickSort()* accepts *mixed[]* (which holds integers or numeric strings at runtime), and all computations became significantly slower.

**How to fix?** Convert *$argv\[1]* to integer:
```php
  if (isset($argv[1])) 
    $arr[] = (int)$argv[1];
```

To prevent such accidents in the future, manually lock return type:
```php
/** @return int[] */
function generateArr(int $size): array { /* ... */ }
```

If you occasionally spoil types, later on, KPHP will show you an error, thanks to *@return*.

```tip
Always think about types, even if you omit them, because [strict typing is performance](../kphp-language/best-practices/strict-typing-performance.md). 
```


## Add abstractions to PHP code

In real life, you use OOP instead of operating data directly.  
Let's extend our example. Instead of `array $arr` and `$arr[$i]` we'll use a wrapping class with getters:
```php
class ArrayHolder {
  /** @var int[] */
  private $arr;

  function __construct(array $arr) {
    $this->arr = $arr;
  }

  function getCount(): int {
    return count($this->arr);
  }

  function getElemAt(int $idx): int {
    return $this->arr[$idx];
  }

  function switchValues(int $idx1, int $idx2) {
    array_swap_int_keys($this->arr, $idx1, $idx2);
  }
};
```

Business logic doesn't change pretty much, it just uses another access to data:
```php
function quickSort2(ArrayHolder $a, int $start = 0, int $last = null) {
  $wall = $start;
  $last = is_null($last) ? $a->getCount() - 1 : $last;
  if ($last - $start < 1)
    return;

  switchValues2($a, (int) (($start + $last) / 2), $last);
  $a_last = $a->getElemAt($last);
  for ($i = $start; $i < $last; $i++)
    if ($a->getElemAt($i) < $a_last) {
      switchValues2($a, $i, $wall);
      $wall++;
    }

  switchValues2($a, $wall, $last);
  quickSort2($a, $start, $wall - 1);
  quickSort2($a, $wall + 1, $last);
}

function switchValues2(ArrayHolder $a, int $i1, int $i2) {
  $a->switchValues($i1, $i2);
}
```


## Compare PHP vs KPHP with abstractions

Let's invoke the code above and compare it with accessing an array directly:

<p class="pay-attention">
  <b>PHP 7.4</b>: ~4600 ms <i>(was 2650)</i><br>  
  <b>KPHP</b>: ~320 ms <i>(was 270)</i>
</p>

Almost nothing changed, but PHP became about 2x slower. That's for the same reason: function calls are expensive in PHP, and fields are more expensive than local variables.
  
KPHP became a bit slower too. Why? Primarily, because KPHP doesn't track nullability for now, and `$o->field` is surrounded with checks that *$o* is not *null*. When KPHP is able to detect nullability, even this small performance degradation will vanish.


```tip
## Conclusion

* when you use accurate types, KPHP is usually faster than PHP
* as your project grows, the gap increases, because simple functions have no overhead in KPHP
```

<div>{%- include navigate-back.liquid -%}</div>
