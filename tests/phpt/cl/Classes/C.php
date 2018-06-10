<?php

namespace Classes;

class C
{
  public $c1 = 1;
  private $c2 = 2;
  /** @var A */
  var $aInst;
  /** @var B */
  private $bInst;
  /** @var C|false */
  public $nextC = false;

  private static $instances = [];

  public function __construct($b1Val = 1, $b2Val = 2) {
    $this->aInst = new A();
    $this->bInst = new B();
  }

  public function setC1($c1) {
    $this->c1 = $c1;
    return $this;
  }

  public function setC2($c2) {
    $this->c2 = $c2;
    return $this;
  }

  /** @return A */
  public function getAInst() {
    return $this->aInst;
  }

  /** @return B */
  public function getBInst() {
    return $this->bInst;
  }

  public function addThis() {
    self::$instances[] = $this;
  }

  public function createNextC() {
    $this->nextC = new self;
    $this->nextC->setC1(4);
    echo $this->nextC->c1, "\n";
  }
}
