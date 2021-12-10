@ok
<?php

class Foo{}

function optional_int(): ?int {
  return 1;
}

function optional_float(): ?float {
  return 1.5;
}

function optional_string(): ?string {
  return "key";
}

function test_valid_keys_on_set() {
  $array = [1, 2, 3];

  $array[true] = 1;
  $array[false] = 1;
  $array[1] = 1;
  $array[1.5] = 1;
  $array["key"] = 1;
  $array[optional_int()] = 1;
  $array[optional_float()] = 1;
  $array[optional_string()] = 1;

  var_dump($array);
}

test_valid_keys_on_set();
