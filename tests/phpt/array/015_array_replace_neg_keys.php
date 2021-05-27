@ok
<?php

class A {
  public $x = "A";
}

/**
 * @param $a A[]
 * @param $b A[]
 */
function test(array $a, array $b) {
  $a = array_replace($a, $b);
  foreach ($a as $k => $v) {
    echo "$k => ", $v->x, "\n";
  }
}

test([], [-2 => new A, -1 => new A, 0 => new A, 1 => new A]);
test([-2 => new A, -1 => new A, 0 => new A, 1 => new A], []);
test([-2 => new A, -1 => new A, 0 => new A, 1 => new A], [-1 => new A, 0 => new A, 1 => new A, 2 => new A]);
