<?php

namespace Classes;

class B
{
  var $a = 'str';
  public $b1 = 1;
  private $b2 = 2;
  var $b3 = 3;

  public static $BBB = 'bbb';

  /**
   * @param int $b1Val
   * @param int $b2Val
   */
  public function __construct($b1Val = 1, $b2Val = 2) {
    $this->b1 = $b1Val;
    $this->b2 = $b2Val;
  }

  public static function createB(int $b1Val = 1, int $b2Val = 2): self {
    return new B($b1Val, $b2Val);
  }

  public function getThis(): self {
    return $this;
  }

  /**
   * @param int $b1
   * @return B
   */
  public function setB1($b1) {
    $this->b1 = $b1;
    return $this;
  }

  public function setB2(int $b2): self {
    $this->b2 = $b2;
    return $this;
  }

  public function setB3(int $b3): self {
    $this->b3 = $b3;
    return $this;
  }
}
