@ok
<?php
require_once 'polyfills.php';

class Foo {
  public function __construct(int $val) { $this->val = $val; }
  public function getVal() : int { return $this->val; }
  private $val = 0;
}

function test_tuple_array() {
  /** @var tuple(Foo, string)[] xs */
  $xs = [
    tuple(new Foo(5), "a"),
    tuple(new Foo(10), "b"),
  ];

  foreach ($xs as [$foo, $s]) {
    var_dump($foo->getVal());
    var_dump($s);
  }

  // Tuples with explicit keys.
  foreach ($xs as [0 => $foo, 1 => $s]) {
    var_dump($foo->getVal());
    var_dump($s);
  }
}

function test_tuple_tuple() {
  /** @var tuple(int, tuple(int, string)[])[] xs */
  $xs = [
    tuple(1, [tuple(10, "a")]),
    tuple(2, [tuple(20, "b")]),
  ];

  foreach ($xs as [$_, $tuples]) {
    foreach ($tuples as list($i, $s)) {
      var_dump($i, $s);
    }
  }
}

test_tuple_array();
test_tuple_tuple();
