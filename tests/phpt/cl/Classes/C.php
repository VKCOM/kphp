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
  /** @var C|null */
  public $nextC = null;

  private static $instances = [];

  /**
   * @param int $b1Val
   * @param int $b2Val
   */
  public function __construct($b1Val = 1, $b2Val = 2) {
    $this->aInst = new A();
    $this->bInst = new B();
  }

  /**
   * @param int $c1
   * @return C
   */
  function setC1($c1) {
    $this->c1 = $c1;
    return $this;
  }

  /**
   * @param int $c2
   * @return C
   */
  function setC2($c2) {
    $this->c2 = $c2;
    return $this;
  }

  /** @return A */
  function getAInst() {
    return $this->aInst;
  }

  /** @return B */
  function getBInst() {
    return $this->bInst;
  }

  function addThis() {
    self::$instances[] = $this;
  }

  function createNextC() {
    $this->nextC = new self;
    $this->nextC->setC1(4);
    echo $this->nextC->c1, "\n";
  }
}
