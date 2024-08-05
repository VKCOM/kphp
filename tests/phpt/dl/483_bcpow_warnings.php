@kphp_runtime_should_warn k2_skip
/First parameter "9\.0E\-5" in function bcpow is not a number/
/First parameter "1\.0E\+14" in function bcpow is not a number/
/First parameter "abcd" in function bcpow is not a number/
/Second parameter "9\.0E\-5" in function bcpow is not a number/
/Second parameter "1\.0E\+14" in function bcpow is not a number/
/Second parameter "abcd" in function bcpow is not a number/
/Second parameter "1000000000000000000" in function bcpow is larger than 1e18/
/Second parameter "-1000000000000000000" in function bcpow is larger than 1e18/
/bcpow\(\): non-zero scale "2\.23" in exponent/
/bcpow\(\): non-zero scale "\-4\.0562" in exponent/
/bcpow\(\): non-zero scale "\-0\.2" in exponent/
/bcpow\(\): non-zero scale "0\.34" in exponent/
<?php

function test_bcpow_bad_args() {
  echo bcpow(9e-5, 2), "\n";
  echo bcpow(1e+14, 2), "\n";
  echo bcpow("abcd", 2), "\n";
  echo bcpow(2, 9e-5), "\n";
  echo bcpow(2, 1e+14), "\n";
  echo bcpow(2, "abcd"), "\n";
}

function test_bcpow_larger_than_1e18() {
  echo bcpow(2, 1000000000000000000), "\n";
  echo bcpow(2, -1000000000000000000), "\n";
}

function test_bcpow_fractional_exp() {
  echo bcpow(2, 2.23), "\n";
  echo bcpow(2, -4.0562), "\n";
  echo bcpow(2, -0.2), "\n";
  echo bcpow(2, 0.34), "\n";
}

test_bcpow_bad_args();
test_bcpow_larger_than_1e18();
test_bcpow_fractional_exp();
