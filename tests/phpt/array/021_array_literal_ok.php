@ok
<?php

function optional_int(): ?int {
  return 1;
}

function optional_float(): ?float {
  return 1.5;
}

function optional_string(): ?string {
  return "key";
}

function test_valid_key_literals() {
  var_dump([true => 1]);
  var_dump([false => 1]);
  var_dump([1 => 1]);
  var_dump([1.5 => 1]);
  var_dump(["key" => 1]);

  var_dump([optional_int() => 1]);
  var_dump([optional_float() => 1]);
  var_dump([optional_string() => 1]);
}

test_valid_key_literals();
