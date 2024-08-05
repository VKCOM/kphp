@ok k2_skip
<?php

function test_var_call() {
  $id = fn($x) => $x;
  var_dump($id(32), $id("str"));
}

function test_array_map() {
  $xs = [2014, 2020];

  // no type hints
  var_dump(array_map(fn($x) => $x*5, $xs));

  // param type hint
  var_dump(array_map(fn(int $x) => $x*5, $xs));

  // result type hint
  var_dump(array_map(fn($x): int => $x*5, $xs));
}

function test_variadic() {
  $varargs = fn(?int... $args): array => $args;
  var_dump($varargs(20, null, 30));
}

function test_implicit_use1(int $param) {
  $local = 10;
  $implicit = 200;

  // (x) <>
  // (shadowed_param) <>
  // (shadowed_local) <>
  // () <implicit>
  // (x) <local>
  // () <param>

  $f1 = fn($x) => $x;
  var_dump($f1(4));

  $f2 = fn($param) => $param;
  var_dump($f2(9));

  $f3 = fn($local) => $local;
  var_dump($f3(11));

  $f4 = fn() => $implicit + $implicit;
  var_dump($f4());

  $f5 = fn($x) => $x + $local;
  var_dump($f5(99));

  $f6 = fn() => $param;
  var_dump($f6());
}

function test_implicit_use2($param1, $param2) {
  $local = 1;

  // (x) (y) <>
  // (x) () <local>
  // () (y) <local>
  // () () <local, param1>
  // () (shadowed_local) <param2>

  $f1 = fn($x) => fn($y) => [$x + $y];
  var_dump($f1(5)(50));

  $f2 = fn($x) => fn() => ['k' => $x + $local];
  var_dump($f2(5)());

  $f3 = fn() => fn($y) => ($y + $local);
  var_dump($f3()(50));

  $f4 = fn() => fn() => $local + $param1;
  var_dump($f4()());

  $f5 = fn() => fn($local) => $local + $param2;
  var_dump($f5()(50));
}

function test_implicit_use3($param1, $param2) {
  $f1 = fn($x, $y) => fn($z) => fn() => $param1 + $param2 * $x * $y + $z;
  var_dump($f1(2, 6)(7)());

  $f2 = fn($x) => fn() => fn($param1) => $param1 + $param2 * $x;
  var_dump($f2(4)()(4000));

  $f3 = fn() => fn($x) => fn($param1) => $param1 + $param2 * $x;
  var_dump($f3()(4)(4000));

  $f4 = fn($x = 10) => fn($y = 20) => fn() => $param1 + $param2 * $x * $y;
  var_dump($f4()()());
}

function test_fnfn() {
  $id = fn($x) => $x;
  $f = fn($x) => $id($x);
  var_dump($f(43));
}

function test_fnfn2() {
  $local = 10;
  $f1 = fn($x) => fn() => $x + $local;
  $f2 = fn($x) => $f1($x)();
  var_dump($f2(43));
}

function test_implicit_fnfn($param) {
  $get_param = fn() => $param;
  $f = fn($x) => $get_param() + $x;
  var_dump($f(43));
}

function make_id_func_of($x) {
  return fn() => $x;
}

function test_make_id_func_of() {
  $f1 = make_id_func_of(10);
  var_dump($f1());

  $f2 = make_id_func_of("foo");
  var_dump($f2());
}

function test_ternary($param) {
  $cond = true;
  $f = fn($x) => $cond ? $param : $x;
  var_dump($f(600));

  // $f() will give the same result as $cond
  // was captured by-value
  $cond = false;
  var_dump($f(600));

  $f2 = fn($x) => $cond ? $param : $x;
  var_dump($f2(600));
}

function test_typed_callable() {
  /** @var callable(string):int */
  $cb = fn($s) => (int)$s;
  var_dump($cb('100500'));

  $arr = [1];
  $cb = fn($s) => strlen($s) + $arr[0];
  var_dump($cb('asdf'));
}

test_var_call();
test_array_map();
test_variadic();
test_implicit_use1(42);
test_implicit_use2(5, 30);
test_implicit_use3(100, 600);
test_fnfn();
test_fnfn2();
test_implicit_fnfn(40);
test_make_id_func_of();
test_ternary(10);
test_typed_callable();
