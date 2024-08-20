@ok k2_skip
<?php

function test_primitive_types() {
#ifndef KPHP
  for ($i = 0; $i < 8; ++$i) {
    var_dump(0);
  }
  return;
#endif
  var_dump(estimate_memory_usage(10));
  var_dump(estimate_memory_usage(1.0));
  var_dump(estimate_memory_usage(null));
  var_dump(estimate_memory_usage(true));
  var_dump(estimate_memory_usage([1, 3, 4, 5]));
  var_dump(estimate_memory_usage("constant string"));
  var_dump(estimate_memory_usage(["constant string", 2, 0.53]));
  var_dump(estimate_memory_usage(tuple(1, "foo", [false, "const"])));
}

function test_string() {
#ifndef KPHP
  var_dump(0);
  var_dump(24);
  var_dump(0);
  var_dump(22);
  var_dump(0);
  var_dump(22);
  var_dump(44);
  return;
#endif
  $x = "hello";
  var_dump(estimate_memory_usage($x));
  $x .= "world!";
  var_dump(estimate_memory_usage($x));
  $x = "empty";
  var_dump(estimate_memory_usage($x));
  $x = "not $x";
  var_dump(estimate_memory_usage($x));
  var_dump(estimate_memory_usage(false ? $x : false));
  var_dump(estimate_memory_usage(true ? $x : false));
  var_dump(estimate_memory_usage(tuple($x, 10, $x)));
}

function test_array() {
#ifndef KPHP
  var_dump(0);
  var_dump(96);
  var_dump(0);
  var_dump(64);
  var_dump(0);
  var_dump(64);
  var_dump(128);
  return;
#endif
  $x = ["hello"];
  var_dump(estimate_memory_usage($x));
  $x[] = ["world!"];
  var_dump(estimate_memory_usage($x));
  $x = ["empty"];
  var_dump(estimate_memory_usage($x));
  $x[1] = $x[0];
  $x[0] = "not";
  var_dump(estimate_memory_usage($x));
  var_dump(estimate_memory_usage(false ? $x : false));
  var_dump(estimate_memory_usage(true ? $x : false));
  var_dump(estimate_memory_usage(tuple($x, 10, $x)));
}

test_primitive_types();
test_string();
test_array();
