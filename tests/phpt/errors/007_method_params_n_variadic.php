@kphp_should_fail
/Not enough arguments.*?Test::f_min1_maxn\] \[found 0\] \[expected at least 1\]/
/Not enough arguments.*?Test::f_min3_maxn\] \[found 2\] \[expected at least 3\]/
/Not enough arguments.*?Test::sf_min1_maxn\] \[found 0\] \[expected at least 1\]/
/Not enough arguments.*?Test::sf_min4_maxn\] \[found 1\] \[expected at least 4\]/
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
