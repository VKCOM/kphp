@kphp_should_fail
/Not enough arguments.*?Test::f_min1_max1\] \[found 0\] \[expected at least 1\]/
/Not enough arguments.*?Test::f_min3_max4\] \[found 1\] \[expected at least 3\]/
/Not enough arguments.*?Test::sf_min1_max1\] \[found 0\] \[expected at least 1\]/
/Too much arguments.*?Test::f_min0_max0\] \[found 3\] \[expected 0\]/
/Too much arguments.*?Test::f_min1_max1\] \[found 2\] \[expected 1\]/
/Too much arguments.*?Test::f_min3_max4\] \[found 5\] \[expected 4\]/
/Too much arguments.*?Test::sf_min0_max0\] \[found 2\] \[expected 0\]/
<?php

class Test {
  public function f_min0_max0() { var_dump(__METHOD__); }
  public function f_min1_max1($x) { var_dump(__METHOD__); }
  public function f_min3_max4($x, $y, $z, $optional = 10) { var_dump(__METHOD__); }

  public static function sf_min0_max0() { var_dump(__METHOD__); }
  public static function sf_min1_max1($x) { var_dump(__METHOD__); }
}

$test = new Test();

// less args than required

$test->f_min1_max1();
$test->f_min3_max4(1);
Test::sf_min1_max1();

// more args than allowed

$test->f_min0_max0(1, 2, 3);
$test->f_min1_max1(1, 2);
$test->f_min3_max4(1, 2, 3, 4, 5);
Test::sf_min0_max0(1, 2);
