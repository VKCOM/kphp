@ok
<?php

require_once 'kphp_tester_include.php';

interface IX {}

class X implements IX {
  public $x = 1;
}

class Y {
  public $y = 2;
}

class XY {
  /** @var tuple(IX,Y) */
  public $t;
}

/**
 * @kphp-infer
 * @return XY
 */
function test_interface_and_array_in_tuple() {
  $xy = new XY;
  $xy->t = tuple(new X, new Y);
  return $xy;
}

test_interface_and_array_in_tuple();
