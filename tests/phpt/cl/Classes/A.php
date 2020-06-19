<?php

namespace Classes;

class A
{
  /** @var any */
  var $a = 0;

  function printA() {
    echo "this->a = ", $this->a, "\n";
  }

  /**
   * @return self
   */
  public function getThis() {
    return $this;
  }

  public function setA($a) {
    $this->a = $a;
    return $this;
  }

  public function createB($b1Val) {
    return new B($b1Val);
  }
}
