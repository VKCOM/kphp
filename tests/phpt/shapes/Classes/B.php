<?php

namespace Classes;

class B
{
  var $a = 'str';
  public $b1 = 1;
  private $b2 = 2;
  var $b3 = 3;

  public static $BBB = 'bbb';

  public function __construct($b1Val = 1, $b2Val = 2) {
    $this->b1 = $b1Val;
    $this->b2 = $b2Val;
  }

  public static function createB($b1Val = 1, $b2Val = 2) {
    return new B($b1Val, $b2Val);
  }

  public function getThis() {
    return $this;
  }

  public function setB1($b1) {
    $this->b1 = $b1;
    return $this;
  }

  public function setB2($b2) {
    $this->b2 = $b2;
    return $this;
  }

  public function setB3($b3) {
    $this->b3 = $b3;
    return $this;
  }
}
