@ok k2_skip
<?php

function get_arg(): string {
  var_dump("get_arg");
  return "foo";
}

function get_string(string $x): string {
   var_dump("get_string");
   return $x;
}

function test_call_rhs() {
  /** @var ?string */
  $x = null;

  $x ??= get_string(get_arg()); // get_string(get_arg()) is evaluated
  var_dump($x);

  $x ??= get_string(get_arg()); // get_string(get_arg()) is not evaluated
  var_dump($x);
}

test_call_rhs();
