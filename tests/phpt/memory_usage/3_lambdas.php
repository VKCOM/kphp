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
  // use($a,$b,$c) is transformed into fields $c,$b,$a (reversed, as FunctionData::uses_list is std::forward_list)
  // counter 4
  $c = false; // +4 + align 0
  $b = 0.2; // +8
  $a = 10; // +4 + align 4
  // total size = 24
  $x = function () use($a, $b, $c) {
    echo "hello world";
  };

#ifndef KPHP
  var_dump(24);
  return;
#endif
  var_dump(estimate_memory_usage($x));
}

function test_lambda_with_use_2() {
  // the same as above, but use($c,$b,$a), which is { $a, $b, $c } fields
  // counter 4 + unique index 4
  $a = 10; // 8
  $b = 0.2; // 8;
  $c = false; // 1 aligned 8
  // total size = 32
  $x = function () use($c, $b, $a) {
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
test_lambda_with_use_2();
