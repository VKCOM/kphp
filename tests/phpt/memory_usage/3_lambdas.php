@ok
<?php

function test_empty_lambda() {
  $x = function () {
    echo "hello world";
  };

#ifndef KPHP
  var_dump(0);
  return;
#endif
  var_dump(estimate_memory_usage($x));
}

function test_lambda_with_use() {
  // counter 4 + unique index 4
  $a = 10; // 8
  $b = 0.2; // 8;
  $c = false; // 1 aligned 8
  // total size = 32
  $x = function () use($a, $b, $c) {
    echo "hello world";
  };

#ifndef KPHP
  var_dump(32);
  return;
#endif
  var_dump(estimate_memory_usage($x));
}

test_empty_lambda();
test_lambda_with_use();
