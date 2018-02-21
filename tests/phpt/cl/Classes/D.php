<?php

namespace Classes;

class D
{
  /** @var C */
  var $cInst;
  /** @var C[] */
  var $cArr;

  public function __construct() {
    $this->cInst = new C();
    $this->cArr = array(new C());
  }

  /** @return C[] */
  public function getCArr() {
    return $this->cArr;
  }
}
