@kphp_runtime_should_warn k2_skip
/bcsqrt\(\): Square root of negative number/
/bcsqrt\(\): Square root of negative number/
/bcsqrt\(\): Square root of negative number/
/First parameter "9\.0E\-5" in function bcsqrt is not a number/
/First parameter "1\.0E\+14" in function bcsqrt is not a number/
/First parameter "abcd" in function bcsqrt is not a number/
<?php

function test_sqrt_of_negative_number() {
  echo bcsqrt(-0.0001), "\n";
  echo bcsqrt(-0.1), "\n";
  echo bcsqrt(-1), "\n";
}

function test_sqrt_of_bad_numbers() {
  echo bcsqrt(9e-5), "\n";
  echo bcsqrt(1e+14), "\n";
  echo bcsqrt("abcd"), "\n";
}

test_sqrt_of_negative_number();
test_sqrt_of_bad_numbers();
