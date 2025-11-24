@ok
<?php

$tests = [
  // Valid dates
  [1, 1, 2009],
  [12, 31, 2009],
  [7, 2, 1963],
  [5, 31, 2009],
  [2, 28, 2009], // non-leap year
  [2, 29, 2008], // leap year
  [7, 2, 1],     // min year
  [7, 2, 32767], // max year

  // Invalid dates
  [13, 1, 2009],
  [2, 31, 2009],
  [1, 32, 2009],
  [2, 29, 2009], // non-leap year
  [7, 2, 32768], // >max year
  [7, 2, 0], // <min year
  [0, 0, 0],
];

date_default_timezone_set("Europe/Moscow");

foreach ($tests as $test) {
  [$m, $d, $y] = $test;
  var_dump(checkdate($m, $d, $y));
  var_dump(checkdate($y, $m, $d));
  var_dump(checkdate($d, $m, $y));
}
