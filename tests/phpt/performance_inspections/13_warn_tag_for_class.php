@kphp_should_warn k2_skip
/variable \$y1 can be reserved with array_reserve functions family out of loop/
/Performance inspection 'array\-reserve' enabled by: Test::member_function/
/expression \$x = array_merge\(\$x, <...>\) can be replaced with array_merge_into\(\$x, <...>\)/
/Performance inspection 'array\-merge\-into' enabled by: Test::static_function/
<?php

/**
 * @kphp-warn-performance array-reserve array-merge-into
 */
class Test {
  public function member_function() {
    $y1 = [];
    for ($i = 0; $i != 1000; ++$i) {
      $y1[] = $i;
    }
    return $y1;
  }

  static public function static_function() {
    $x = [1, 2, 3 ,4];
    if (1) {
      $x = array_merge($x, [1]);
    }
    return $x;
  }
}

function test() {
  (new Test)->member_function();
  Test::static_function();
}

test();
