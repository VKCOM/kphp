@ok
<?php
require_once 'kphp_tester_include.php';

class Foo {
  public function __construct(int $val) { $this->val = $val; }
  public function getVal() : int { return $this->val; }
  private $val = 0;
}

function test_shape_array() {
  /** @var shape(foo:Foo, s:string)[] xs */
  $xs = [
    shape(['foo' => new Foo(5), 's' => "a"]),
    shape(['foo' => new Foo(10), 's' => "b"]),
  ];

  foreach ($xs as ['foo' => $foo, 's' => $s,]) {
    var_dump($foo->getVal());
    var_dump($s);
  }
}

test_shape_array();
