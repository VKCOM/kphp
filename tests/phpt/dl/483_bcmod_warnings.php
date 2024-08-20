@kphp_runtime_should_warn k2_skip
/First parameter "9\.0E\-5" in function bcmod is not a number/
/First parameter "1\.0E\+14" in function bcmod is not a number/
/First parameter "abcd" in function bcmod is not a number/
/Second parameter "9\.0E\-5" in function bcmod is not a number/
/Second parameter "1\.0E\+14" in function bcmod is not a number/
/Second parameter "abcd" in function bcmod is not a number/
/Second parameter "" in function bcmod is not a number/
/bcmod\(\): Division by zero/
/bcmod\(\): Division by zero/
/bcmod\(\): Division by zero/
/bcmod\(\): Division by zero/
/bcmod\(\): Division by zero/
<?php

function test_bcmod_bad_numbers() {
  echo bcmod(9e-5, 2), "\n";
  echo bcmod(1e+14, 2), "\n";
  echo bcmod("abcd", 2), "\n";
  echo bcmod(2, 9e-5), "\n";
  echo bcmod(2, 1e+14), "\n";
  echo bcmod(2, "abcd"), "\n";
  echo bcmod(2, ""), "\n";
}

function test_bcmod_division_by_zero() {
  echo bcmod(2, 0), "\n";
  echo bcmod(2, 0.0), "\n";
  echo bcmod(2, -0.0), "\n";
  echo bcmod(2, +0.0), "\n";
  echo bcmod(2, +00.00000), "\n";
}

test_bcmod_bad_numbers();
test_bcmod_division_by_zero();
