@ok
<?php

class Example {
  public function test_this1($param) {
    $f = fn($x) => $this->x + $x + $param;
    var_dump($f(50));
  }

  public function test_this2() {
    $f = fn($x) => $this->add1($x);
    var_dump($f(10), $f(20));
  }

  private function add1(int $x): int { return $x + 1; }

  public $x = 10;
}

$obj = new Example();
$obj->test_this1(4);
$obj->test_this2();
