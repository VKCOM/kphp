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

$low_nums = [-0, +0, 0, 0.1234046, 0.123, 0.1, 0.0, 0., .0, 0.0001, 0.2, 0.2001, 0.000100000345, 00.01, 000.1000305020100];

$powers = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99, 100];
test_bcpow($low_nums, $powers);

