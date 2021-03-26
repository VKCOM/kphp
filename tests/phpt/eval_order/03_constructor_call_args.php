@ok
<?php

require_once 'kphp_tester_include.php';

/** @param int[] $v */
function arg(array $v): int {
  var_dump($v);
  return array_first_value($v);
}

class F {
  public function __construct(int $x, int $y, int $z) {}
}

class G {
  public function __construct(int $x, int $y) {}
}

function test() {
  $f = new F(1, 2, 3);
  $f = new F(arg([__LINE__ => 1]), 3, 4);
  $f = new F(1 , 2, arg([__LINE__ => 3]));

  $f = new F(arg([__LINE__ => 9]), arg([__LINE__ => 8]), arg([__LINE__ => 7]));
  $f = new F(arg([__LINE__ => 7]), arg([__LINE__ => 8]), arg([__LINE__ => 9]));

  $g = new G(arg([__LINE__ => 1]), arg([__LINE__ => 2]));
  $g = new G(arg([__LINE__ => 2]), arg([__LINE__ => 1]));
}

test();
