<?php

function test_bcpow($nums, $powers) {
  foreach ($nums as $base) {
    foreach ($powers as $power) {
      echo bcpow($base, $power), "\n";
      echo bcpow(-$base, $power), "\n";
      echo bcpow($base, -$power), "\n";
      echo bcpow(-$base, -$power), "\n";

      for ($i = 0; $i < 20; ++$i) {
        echo bcpow($base, $power, $i), "\n";
        echo bcpow(-$base, $power, $i), "\n";
        echo bcpow($base, -$power, $i), "\n";
        echo bcpow(-$base, -$power, $i), "\n";
      }
    }
  }
}

$mid_nums = ["", 1, 1.0, 1.00, 1.01, 1.001, 2, 2.0, 2.00, 2.01, 2.001, 2.0001, 2.00010003, 3, 4, 5, 6, 7, 8, 9,
             9.12342135234234234, 9.12342135, 9.123, 9.1, 9.10, 9.100, 9.0, 9.00, 09.00, 009.00, 009.001, 9.00000001, 9.000000013445];

$powers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99, 100];
test_bcpow($mid_nums, $powers);
