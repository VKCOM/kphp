@ok
<?php

function foo(int $i) {
  var_dump($i);
}

function test_shorthand_ternary($a) {
  if (0) {
    $a = false;
  }
  $b = $a ?: 321;
  foo($b);
}

function test_opset_inside_condition($a) {
  if (0) {
    $a = false;
  }
  if ($b = $a) {
    foo($b);
  }
}

test_shorthand_ternary(123);
test_opset_inside_condition(123);
