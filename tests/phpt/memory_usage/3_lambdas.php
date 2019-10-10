@ok
<?php

function test_empty_lambda() {
  $x = function () {
    echo "hello world";
  };

#ifndef KittenPHP
  var_dump(0);
  return;
#endif
  var_dump(estimate_memory_usage($x));
}

function test_lambda_with_use() {
  // counter 4
  $a = 10; // 4
  $b = 0.2; // 8;
  $c = false; // 1
  // total size = 24
  $x = function () use($a, $b, $c) {
    echo "hello world";
  };

#ifndef KittenPHP
  var_dump(24);
  return;
#endif
  var_dump(estimate_memory_usage($x));
}

test_empty_lambda();
test_lambda_with_use();
