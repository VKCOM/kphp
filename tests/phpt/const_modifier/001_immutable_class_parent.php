@ok
<?php

/**
 * @kphp-immutable-class
 */
class A {
  /**
   * @param int $x
   */
  public function __construct($x) {
    $this->x = $x;
  }

  public $x = 1;
}

/**
 * @kphp-immutable-class
 */
class B extends A {
  /**
   * @param int $y
   * @param int $x
   */
  public function __construct($y, $x) {
    parent::__construct($x);
    $this->y = $y;
  }

  public $y = 1;
}

class C extends A {
  /**
   * @param int $x
   */
  public function __construct($x) {
    parent::__construct($x);
  }

  public $y = 1;
}

$b = new B(76, 12);
var_dump($b->x);
var_dump($b->y);

$c = new C(99);
$c->y = 21;
var_dump($c->x);
var_dump($c->y);
