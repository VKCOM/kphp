@kphp_should_fail k2_skip
/class B must be serializable as it is used in field A::b/
<?php
require_once 'kphp_tester_include.php';

class B {
  /** @var int */
  public $x = 0;
}

/** @kphp-serializable */
class A {
  /**
   * @kphp-serialized-field 1
   * @var \tuple(B)
   */
  public $b;

  public function __construct() {
    $this->b = tuple(new B());
  }
}

instance_serialize(new A());

