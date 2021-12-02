@ok
<?php

function test_array_unique_as_string($array) {
  var_dump(array_unique($array, SORT_STRING));
  var_dump(array_unique($array)); // SORT_STRING used by default
}

function test_array_unique_as_numeric($array) {
  var_dump(array_unique($array, SORT_NUMERIC));
}

function test_array_unique_no_conversion($array) {
  var_dump(array_unique($array, SORT_REGULAR));
}

function test_array_unique_no_conversion_php_kphp_diff() {
  #ifndef KPHP
  if (false) {
  #endif
    test_array_unique_no_conversion([false, null]);
    test_array_unique_no_conversion([0, null]);
    test_array_unique_no_conversion([2.12, "2.12"]);
  #ifndef KPHP
  } else {
    var_dump([false, null]);
    var_dump([0, null]);
    var_dump([2.12, "2.12"]);
  }
  #endif
}

test_array_unique_as_string(["red", "color" => "red", 78, 78, 12, "12", false, null, false, 0, true, 1, 0, null, 2.12, "2.12", 3.0, 3]);
test_array_unique_as_numeric(["red", "color" => "red", 78, 78, 12, "12", false, null, false, 0, true, 1, 0, null, 2.12, "2.12", 3.0, 3]);
test_array_unique_no_conversion(["red", "color" => "red", 78, 78, 12, "12", false, 0, 3.0, 3]);
test_array_unique_no_conversion_php_kphp_diff();
