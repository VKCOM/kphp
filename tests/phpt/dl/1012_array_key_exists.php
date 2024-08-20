@ok k2_skip
<?php

function test_int_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_string_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_double_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_bool_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_or_false_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_array_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

function test_mixed_keys($hint, $key, $arr) {
  echo "$hint: "; var_dump(array_key_exists($key, $arr));
}

$arrays = [
  [], // array0
  [1 => "1"], // array1
  [1 => "1", "hello" => "world", 0 => "0"], // array2
  ["" => "empty", false => "false"], // array3
  ["hello" => "world", "" => "empty", true => "true", null => "null"], // array4
  [1 => "1", "hello" => "world", "" => "empty", false => "false", true => "true", null => "null"], // array5
];

for ($i = 0; $i < count($arrays); ++$i) {
  foreach([1, 0, 2, 9912] as $int_key) {
    test_int_keys("test_int_keys array$i, key=$int_key : ", $int_key, $arrays[$i]);
  }

  foreach(["1", "hello", "0", "world"] as $string_key) {
    test_string_keys("test_string_keys array$i, key='$string_key' : ", $string_key, $arrays[$i]);
  }

  foreach(["0.0" => 0.0, "1.0" => 1.0, "1.2" => 1.2, "0.4" => 0.4, "0.99" => 0.99,
           "0.01" => 0.01, "nan" => 0.0/0.0, "inf" => 1.0/0.0, "-inf" => -1.0/0.0] as $key_hint => $double_key) {
    test_double_keys("test_double_keys array$i, key=$key_hint : ", $double_key, $arrays[$i]);
  }

  foreach([true, false] as $bool_key) {
    test_bool_keys("test_bool_keys array$i, key='$bool_key' : ", $bool_key, $arrays[$i]);
  }

  foreach([true, false] as $bool_key) {
    test_bool_keys("test_bool_keys array$i, key='$bool_key' : ", $bool_key, $arrays[$i]);
  }

  foreach(["hello", false] as $or_false_key) {
    test_or_false_keys("test_or_false_keys array$i, key='$or_false_key' : ", $or_false_key, $arrays[$i]);
  }

  foreach(["[]" => [], "[1, 2]" => [1, 2], "[7]" => [7]] as $key_hint => $array_key) {
    test_array_keys("test_array_keys array$i, key='$key_hint' : ", $array_key, $arrays[$i]);
  }

  foreach(["hello" => "hello", "false" => false, "1" => 1, "99" => 99,
           "0.0" => 0.0, "[1]" => [1], "null" => null, "-9" => -9] as $key_hint => $mixed_key) {
    test_mixed_keys("test_mixed_keys array$i, key='$key_hint' : ", $mixed_key, $arrays[$i]);
  }
}
