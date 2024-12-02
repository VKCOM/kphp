@ok
<?php

function test_mt_getrandmax() {
  var_dump(mt_getrandmax());
}

function test_mt_rand() {
  $x = mt_rand();
  var_dump($x >= 0 && $x <= mt_getrandmax());

  $x = mt_rand(100, 500);
  var_dump($x >= 100 && $x <= 500);

  $x = mt_rand(-450, -200);
  var_dump($x >= -450 && $x <= -200);

  $x = mt_rand(-124, 107);
  var_dump($x >= -124 && $x <= 107);

  $x = mt_rand(0, mt_getrandmax());
  var_dump($x >= 0 && $x <= mt_getrandmax());

  $x = mt_rand(-mt_getrandmax(), 0);
  var_dump($x >= -mt_getrandmax() && $x <= 0);

  $x = mt_rand(-mt_getrandmax(), mt_getrandmax());
  var_dump($x >= -mt_getrandmax() && $x <= mt_getrandmax());

  $x = mt_rand(0, PHP_INT_MAX);
  var_dump($x >= 0 && $x <= PHP_INT_MAX);

  $x = mt_rand(-PHP_INT_MAX - 1, 0);
  var_dump($x >= -PHP_INT_MAX - 1 && $x <= 0);

  $x = mt_rand(-PHP_INT_MAX - 1, PHP_INT_MAX);
  var_dump($x >= (-PHP_INT_MAX - 1) && $x <= PHP_INT_MAX);

  var_dump(mt_rand(0, 0));
  var_dump(mt_rand(1, 1));
  var_dump(mt_rand(-4, -4));
  var_dump(mt_rand(9287167122323, 9287167122323));
  var_dump(mt_rand(-8548276162, -8548276162));

  var_dump(mt_rand(mt_getrandmax(), mt_getrandmax()));
  var_dump(mt_rand(-mt_getrandmax(), -mt_getrandmax()));
  var_dump(mt_rand(PHP_INT_MAX, PHP_INT_MAX));
  var_dump(mt_rand(-PHP_INT_MAX - 1, -PHP_INT_MAX - 1));

  // PHP вернет false, KPHP вернет 0
  var_dump(!mt_rand(1, -1));
  var_dump(!mt_rand(721, -42));
  var_dump(!mt_rand(mt_getrandmax(), -mt_getrandmax()));
  var_dump(!mt_rand(PHP_INT_MAX, -PHP_INT_MAX - 1));
}

function gen_rand_array() : array {
  $arr = [];
  array_push($arr, mt_rand());
  array_push($arr, mt_rand(42, 42));
  array_push($arr, mt_rand(84721, 48917223));
  array_push($arr, mt_rand(-4123, -12));
  array_push($arr, mt_rand(-788, 4214));
  array_push($arr, mt_rand(0, mt_getrandmax()));
  array_push($arr, mt_rand(-mt_getrandmax(), 0));
  array_push($arr, mt_rand(-mt_getrandmax(), mt_getrandmax()));
  array_push($arr, mt_rand(0, PHP_INT_MAX));
  array_push($arr, mt_rand(-PHP_INT_MAX - 1, 0));
  array_push($arr, mt_rand(-PHP_INT_MAX - 1, PHP_INT_MAX));
  return $arr;
}

function test_mt_srand() {
  mt_srand(1);
  $a1 = gen_rand_array();

  mt_srand(242);
  $a242 = gen_rand_array();

  mt_srand(1);
  $b1 = gen_rand_array();

  mt_srand(242);
  $b242 = gen_rand_array();

  var_dump($a1 === $b1);
  var_dump($a242 === $b242);
}


test_mt_getrandmax();
test_mt_rand();
test_mt_srand();
