@ok
<?php

require_once 'polyfills.php';

interface IX {}

class X implements IX {
  public $x = 1;
}

class Y {
  public $y = 2;
}

class XY {
  /** @var shape(ix:IX, y:Y) */
  public $t;
}

/**
 * @kphp-infer
 * @return XY
 */
function test_interface_and_array_in_shape() {
  $xy = new XY;
  $xy->t = shape(['ix' => new X, 'y' => new Y]);
  return $xy;
}

test_interface_and_array_in_shape();
