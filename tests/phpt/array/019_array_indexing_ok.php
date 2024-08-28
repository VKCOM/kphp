@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function optional_int(): ?int {
  return 1;
}

function optional_float(): ?float {
  return 1.5;
}

function optional_string(): ?string {
  return "key";
}

function test_valid_indexing() {
  $array = [1, 2, 3, "key" => "value"];

  echo $array[true];
  echo $array[false];
  echo $array[1];
  echo $array[1.5];
  echo $array["key"];

  echo $array[optional_int()];
  echo $array[optional_float()];
  echo $array[optional_string()];
}

function test_feature_keys() {
  $future = fork(function() { return 1; } );
  $array = [$future => 1];
  wait($future);
}

test_valid_indexing();
test_feature_keys();
