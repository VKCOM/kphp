@ok
<?php
function test_array_rand() {
  $array = [1, 2, 3];
  $result = array_rand($array, 3);
  if (is_array($result) && count($result) === 3) {
    var_dump($result);
  }
}

function test_array_rand_no_num_val() {
  $array = [1, 2, 3];
  $result = array_rand($array);
  if (is_int($result)) {
    var_dump('int result');
  }
}

function test_array_rand_null_zero_num() {
  $array = [1, 2, 3];
  $result = @array_rand($array, 0);
  if ($result === null) {
    var_dump('result is null, cause num is equals to zero');
  }
}

function test_array_rand_null_empty_array() {
  $array = [];
  $result = @array_rand($array, 2);
  if ($result === null) {
    var_dump('result is null, cause array is empty');
  }
}

function test_array_rand_null_num_greater_then_array_size() {
  $array = [1, 2];
  $result = @array_rand($array, 3);
  if ($result === null) {
    var_dump('result is null, cause num is greater then len of array size');
  }
}

test_array_rand();
test_array_rand_no_num_val();
test_array_rand_null_zero_num();
test_array_rand_null_empty_array();
test_array_rand_null_num_greater_then_array_size();
