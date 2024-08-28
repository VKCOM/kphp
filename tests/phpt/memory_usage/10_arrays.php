@ok k2_skip
<?php

function test_vector() {
#ifndef KPHP
  // sizeof(array_inner_control) + buf_size * sizeof(int64_t)
  var_dump(32 + 2 * 8);
  var_dump(32 + 2 * 8);
  var_dump(32 + 4 * 8);
  var_dump(32 + 4 * 8);
  var_dump(32 + 8 * 8);
  var_dump(32 + 8 * 8);
  var_dump(32 + 8 * 8);
  var_dump(32 + 8 * 8);
  var_dump(32 + 16 * 8);
  var_dump(32 + 16 * 8);
  return;
#endif
  $v = [];
  for ($i = 0; $i < 10; ++$i) {
    $v[] = 42;
    var_dump(estimate_memory_usage($v));
  }
}

function test_map() {
#ifndef KPHP
  // sizeof(array_inner_fields_for_map) + sizeof(array_inner_control) + buf_size * sizeof(array_bucket)
  var_dump(16 + 32 + 11 * 32);
  var_dump(16 + 32 + 33 * 32);
  return;
#endif
  $m = [];
  for ($i = 0; $i < 7; ++$i) {
    $m[$i + 1] = 42;
  }
  var_dump(estimate_memory_usage($m));
  $m[23] = 42;
  var_dump(estimate_memory_usage($m));
}

test_vector();
test_map();
