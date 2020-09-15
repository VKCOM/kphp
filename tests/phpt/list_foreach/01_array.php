@ok
<?php
require_once 'polyfills.php';

class Foo {
  public function __construct(int $val) { $this->val = $val; }
  public function getVal() : int { return $this->val; }
  private $val = 0;
}

function test_int_array2() {
  /** @var int[][] $xs */
  $xs = [[1], [5, -1], [1, 0], [7, 5]];
  $dst = [0];

  foreach ($xs as list($dst[0], )) {
    var_dump($dst);
  }
}

function test_mixed_array() {
  $xs = [
    [[["x"], 54], [1, "y"], [[], null, false]],
    [[[""], 4.5, true], [], []],
  ];

  foreach ($xs as list($x, $y, $z,)) {
    foreach ($x as list($v)) {
      var_dump($v);
    }
    var_dump($y, $z);
  }
}

test_int_array2();
test_mixed_array();
