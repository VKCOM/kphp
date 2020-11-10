---
noprevnext: true
notopnavigation: true
---

# Type inferring examples

Here we have some snippets of PHP code, and what types are inferred afterwards.  


#### lca(int, double) = double

```php
$a = 4;
$a += 5.5;
 
// $a :: double
```

#### lca(int, string) = mixed

```php
$a = 4;
$a += '5.5';
 
// $a :: mixed
```

#### int + mixed = mixed

```php
$a = 4;
$b = 5 + '5.5';
$c = $a + $b;
 
// $a :: int
// $b :: mixed => $c :: mixed
```

#### Concatenation is string

```php

$a = 4;
$b = 5 + '5.5';
$c = $a . $b;
 
// $a :: int, $b :: mixed
// but $c :: string
```   

#### All elements of an array have one type — array&lt;T&gt;

```php
// array<string>
$a = ['1', '2', (string)3];
 
// array<double>
$num = floatval($_GET['val']);
$a = [1.5, -100];
$a[] = $num;
 
// array<array<int>>
$a = [ [1,2], [3,4,5=>5], ['key' => (int)'7']];
 
// array<array<double>>
$a = [];
$a[$i1][$i2] = 5.4;
```

#### Mixing types inside an array — array&lt;mixed&gt;

```php
// array<mixed>
$a = [1, 2, '3'];
 
// array<mixed>
$a = [1,2];
$a[] = $_GET['val'];
 
// array<array<mixed>>
$a = [ [1,2], [3,4,5=>5], ['key' => '7']];
 
// array<mixed>
return [$count, $items_arr];
 
// array<mixed>
['string', [1,2]];
 
// array<mixed>, as $config is probably array<mixed>
$options = ['id' => true, 'num' => true, 'idx' => $config['idx']];
 
// array<mixed>, as $options is probably not array<bool>
$def = ['id' => true] + $options;
```

#### Mixing primitive types — mixed

```php
// mixed
$a = isset($input['v']) ? $input['v'] : 0;
 
// mixed
$a = [1,2,3];
if (...) {
  $a = 4;
  echo $a;
}
 
// mixed
if (...) {
  $a = 'str';
  ...
} else {
  $a = [];
}
echo $a;
 
// mixed
$val = $config['key'];
 
// get() returns mixed[] => $count :: mixed, $items :: mixed
function get() { ... return [$count, $items]; }
list($count, $items) = get();       

// default value int, called with bool => $arg :: mixed
function f3($arg = 42) { }
f3(true);
```

#### Array key — mixed

```php
$arr = [...];
foreach ($arr as $key => $v) {
  // $key :: mixed  (since keys are int|string in PHP)
  // $arr :: array<T> => $v :: T
}
```

#### T | null (or ?T, shorter form) (nullability doesn't convert to mixed)

```php
// ?int
$v = ... ? 0 : null;
 
// ?array<int>
$a = null;
if (...) {
  $a = fReturningArrayInt();
}

// mixed, as null|mixed = mixed
$a = null;
if (...) {
  $a = fReturningMixed();
}

// returns ?bool
function f2($id) {
  return $id == 0 ? null : $id > 0;
}
```

#### T | false (falseability doesn't convert to mixed)

```php
// int|false
$v = ... ? 0 : false;

// string|false, as substr() can return false
$s = substr("string", 0, 1);

// int|false, as strpos() can return false
$pos = strpos($haystack, $needle, $offset);

// int|false|null
$pos = 1 ? strpos(...) : null;        

// (int|false)[]
$arr = [1, 2, false, getInt(), (int)$_GET['id']];        

// $user_id :: int|false
function log($user_id = false) {}
log(42);
```

#### Empty arrays have Unknown inside

```php
// $options :: array<Unknown>, as it is always empty from all invocations
function log($user_id, $options = []) {
  // this is an error! $v will be Unknown type
  $v = $options['use_newlines'];
  // foreach on always-empty array is also an error
}

log(1);
log(2);
log(3, []);
```

#### array_* functions leave types as is

```php
// array<int>
$a2 = array_values(['a' => 1, 'b' => 2]);
 
// array<string>
$a = array_reverse(['c','b','a']);
 
// $v :: string, $a remains array<string> 
$v = array_shift($a);
 
// array<double>
$a = array_fill(0, 10, (float)$input['v']);
```

#### References change argument types

```php
function assignTo(&$target, $value) {
  $target = $value;
}

$t1 = 0;
assignTo($t1, 100);

$t2 = '0';
assignTo($t2, '100');

// everything is mixed:
// 1) $value is int|string => $value :: mixed
// 2) $t1 and $t2 by ref assigned mixed => $t1 :: mixed, $t2 :: mixed
// 3) $t1 and $t2 passed to $target => $target :: mixed 
```

```tip
KPHP has many types except primitives: instances, tuples, shapes, futures...  
Type inferring concepts are the same for all of them.   
```

<div>{%- include navigate-back.liquid -%}</div>
