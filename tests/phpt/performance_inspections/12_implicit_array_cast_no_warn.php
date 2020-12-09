@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

/**
 * @param $x mixed
 */
function call_with_implicit_mixed_cast($x) {
  var_dump($x);
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test() {
  call_with_implicit_mixed_cast(123);

  $a = [1, "2", [1, 2, 3]];
  call_with_implicit_mixed_cast($a);

  $b = [];
  call_with_implicit_mixed_cast($b);

  $c = 1 ? [1] : "x";
  $c[] = $a;
  $c["xx"] = $b;

  $c[] = [$a, $b];

  if (0) {
    $c = $a;
    if (0) {
      return $b;
    } else if (false) {
      return $a;
    } else if (null) {
      return [1, 2, 3];
    }
  }
  return $c;
}

test();
