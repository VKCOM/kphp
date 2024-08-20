@ok php8 k2_skip
<?php

function test_random_int() {
  $x = random_int(100, 500);
  var_dump($x >= 100 && $x <= 500);

  $x = random_int(-450, -200);
  var_dump($x >= -450 && $x <= -200);

  $x = random_int(-124, 107);
  var_dump($x >= -124 && $x <= 107);

  $x = random_int(0, PHP_INT_MAX);
  var_dump($x >= 0 && $x <= PHP_INT_MAX);

  $x = random_int(-PHP_INT_MAX - 1, 0);
  var_dump($x >= -PHP_INT_MAX - 1 && $x <= 0);

  $x = random_int(-PHP_INT_MAX - 1, PHP_INT_MAX);
  var_dump($x >= (-PHP_INT_MAX - 1) && $x <= PHP_INT_MAX);

  var_dump(random_int(0, 0));
  var_dump(random_int(1, 1));
  var_dump(random_int(-4, -4));
  var_dump(random_int(9287167122323, 9287167122323));
  var_dump(random_int(-8548276162, -8548276162));

  var_dump(random_int(PHP_INT_MAX, PHP_INT_MAX));
  var_dump(random_int(-PHP_INT_MAX - 1, -PHP_INT_MAX - 1));
}

test_random_int();
