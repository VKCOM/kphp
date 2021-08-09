<?php

namespace ReferenceInvariant;

class TestClassAB extends TestAbstractClassA implements TestInterfaceB {
  function __construct() {
    $b = new TestClassB(200);
    parent::__construct($this, new TestClassA(100), $this, $b);

    for ($i = 0; $i != 123; ++$i) {
      $this->b_arr["key $i"] = $b;
    }
  }

  /** @var TestInterfaceB[] */
  public $b_arr = [];
}
