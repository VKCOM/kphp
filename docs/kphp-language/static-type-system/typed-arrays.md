---
sort: 5
---

# Typed arrays

```danger
Getting values from typed arrays an important behavior difference between PHP and KPHP.  
```


## Getting an unexisting element is T, not null

When you have `T[]`, reading by any index infers as `T`. So, reading unexisting returns **empty *T***:

```php
$a = [1,2,3];
$int = $a[100];      // int (0)
["a", "b"][999];     // string ""
$tuples = [tuple(5, "a")];
$tuples['asdf'];     // tuple(0, "")
$int2 = [[1],[2]];   // int[][]
$int2[100];          // array()
```

If *T* can handle *null*, then this problem doesn't exist. For instance, objects and mixed are nullable:
```php
$a = [1, 2, null, 4];      // (int|null)[]
$a[100];                   // null => as in PHP
[new User][100];           // null => as in PHP
$mixed_arr['unexisting'];  // null => as in PHP
```

```note
This behavior exists on purpose, because if indexing `T[]` would be `?T`, it would tremendously spoil all types.
```


## Getting an existing element is T

This case can be found in *float[]* which mixes floats and ints:

```php
$a = [1, 2, 4.12];   // float[]
$a[0];               // float (1.0)! not int (1)!
``` 


## Typically, it is not a problem (surprising!)

When you get *0* instead of *null*, you typically compare it / sum it up / etc. ***null* often behaves like empty *T***:
```php
$v = $int_arr[100500];  // 0 in KPHP, null in PHP
if ($v) ;               // equal (false)
if ($v > 1.2) ;         // equal (false)
$v == 0;                // equal (true) (since ==, not ===)
$v + 3.5;               // equal (3.5)
10 << $v;               // equal (10)

$s = $str_arr[100500];  // "" in KPHP, null in PHP
"style='$s'";           // equal (style='')
$s[0];                  // equal ('')
strlen($s);             // equal (0)

$a = $arr_arr[100500];  // [] in KPHP, null in PHP
count($a);              // equal (0)
if ($a);                // equal (false)
if ($a['some_idx']);    // equal (false)
isset($a[0]);           // equal (false)
``` 


## If the result may differ, KPHP produces compilation errors

Examples above work identically, regardless of different actual values in PHP/KPHP.  
Here are examples that produce different results because of this problem:

```php
$v = $float_arr[0];        // 1.0 in KPHP, 1 in PHP
is_float($v);              // would work differently

$v = $int_arr[100500];     // 0 in KPHP, null in PHP
$v === 0;                  // would work differently (since ===, not ==)
```

**KPHP is able to find such cases and produce an error**.

Example — and an error:
```php
$v = $float_arr[0];
is_float($v);
```
```
//   6:is_float($v);
isset, !==, ===, is_array or similar function result may differ from PHP
 Probably, this happened because as expression:  $float_arr[.] : float  at dev.php: demo:5 of type double can't be null in KPHP, while it can be in PHP
```

Example 2 — and a similar error:
```php
$v = $int_arr[100500];
$v === 0;
```
```
//   9: $v === 0;
isset, !==, ===, is_array or similar function result may differ from PHP
 Probably, this happened because as expression:  $int_arr[.] : int  at dev.php: demo:9 of type int can't be null in KPHP, while it can be in PHP
```

But still, *echo* and some others would work differently. In practice, it's very unlikely to come across such misbehavior.


## How to deal with this error '... result may differ from PHP'?

This error protects you from getting an unexisting index. Real-life example:
```php
function getMarkupElements(): array {
  $html = '<div></div>';
  $js = '';
  return ['html' => $html, 'js' => $js];
}

function renderUI(array $markup): string {
  $js = $markup['js'];
  if ($js !== '')
    $js = "<script>$js</script>";
  return $markup['html'] . $js;
}

renderUI(getMarkupElements());
```

You try to compile this valid code and get a compilation error:
```
//  11:  if ($js !== '')
isset, !==, ===, is_array or similar function result may differ from PHP
 Probably, this happened because as expression:  $markup[.] : string  at dev.php: renderUI:10 of type string can't be null in KPHP, while it can be in PHP
```

You analyze the error and understand, that *array $markup* was inferred as *string[]*, and `if ($js !== '')` would work differently if `['js']` key doesn't exist in *$markup*.

But **you know, that key exists**. The compiler is right in general, but in this case, you want to suppress this error:
```php
  // either
  $js = (string)$markup['js'];
  // or
  $js = not_null($markup['js']);
```  

Using *not_null()* is more elegant and semantic, it is a preferred solution.

```tip
## Concluding typed arrays
Getting unexisting elements from typed arrays differs in PHP and KPHP, but typically it's not a problem.  
If KPHP finds potential misbehavior, it warns you.  
Sometimes KPHP is correct and you should reconsider your code.  
Sometimes you know better and suppress it with *cast* or *not_null()*. 
```
