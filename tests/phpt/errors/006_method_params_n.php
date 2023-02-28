@kphp_should_fail
/Too few arguments in call to Test::f_min1_max1\(\), expected 1, have 0/
/Too few arguments in call to Test::f_min3_max4\(\), expected 3, have 1/
/Too few arguments in call to Test::sf_min1_max1\(\), expected 1, have 0/
/Too many arguments in call to Test::f_min0_max0\(\), expected 0, have 3/
/Too many arguments in call to Test::f_min1_max1\(\), expected 1, have 2/
/Too many arguments in call to Test::f_min3_max4\(\), expected 4, have 5/
/Too many arguments in call to Test::sf_min0_max0\(\), expected 0, have 2/
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
