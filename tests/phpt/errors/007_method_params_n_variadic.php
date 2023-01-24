@kphp_should_fail
/Too few arguments in call to Test::f_min1_maxn\(\), expected 2, have 0/
/Too few arguments in call to Test::f_min3_maxn\(\), expected 4, have 2/
/Too few arguments in call to Test::sf_min1_maxn\(\), expected 2, have 0/
/Too few arguments in call to Test::sf_min4_maxn\(\), expected 5, have 1/
<?php

// as variadic functions are preprocessed, error is given
// during the different stage; hence the separate test file for them

class Test {
  public function f_min1_maxn($x, ...$rest) { var_dump(__METHOD__); }
  public function f_min3_maxn($x, $y, $z, ...$rest) { var_dump(__METHOD__); }

  public static function sf_min1_maxn($x, ...$rest) { var_dump(__METHOD__); }
  public static function sf_min4_maxn($x, $y, $z, $_, ...$rest) { var_dump(__METHOD__); }
}

$test = new Test();

// less args than required

$test->f_min1_maxn();
$test->f_min3_maxn(1, 2);
Test::sf_min1_maxn();
Test::sf_min4_maxn(1);
