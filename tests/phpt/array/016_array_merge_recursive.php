@ok k2_skip
<?php

function test_merge_vectors() {
  $arr1 = [1, [1, 2]];
  $arr2 = [3, ["hello", 'world']];
  $arr3 = [[6, 7], ["str1", 'str2']];

  var_dump(array_merge_recursive());
  var_dump(array_merge_recursive($arr1));
  var_dump(array_merge_recursive($arr1, $arr2));
  var_dump(array_merge_recursive($arr1, $arr2, $arr3));
}

function test_merge_maps() {
  $arr1 = [1 => "one", 2 => [1, 2]];
  $arr2 = [2 => 'three', "four" => ["hello", 'world']];
  $arr3 = [1 => [6, 7], 'four' => ["str1", 'str2']];

  var_dump(array_merge_recursive());
  var_dump(array_merge_recursive($arr1));
  var_dump(array_merge_recursive($arr1, $arr2));
  var_dump(array_merge_recursive($arr1, $arr2, $arr3));
}

function test_merge_array_in_place() {
  var_dump(array_merge_recursive([]));
  var_dump(array_merge_recursive([], []));
  var_dump(array_merge_recursive([1, 2], ["foo" => "bar"]));
  var_dump(array_merge_recursive(["ids" => ["name" => ["Andrew", "Bill"]]], ["ids" => ["name" => "Mark"]]));
}

test_merge_vectors();
test_merge_maps();
test_merge_array_in_place();
