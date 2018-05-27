<?php

namespace Classes;

class Z4BadPhpdoc
{
  /** @var int */
  public $a = 0;
  /** @var Z4BadPhpdoc|false */
  public $zInst;

  public function __construct() {

  }

  public function createZ() {
    $this->zInst = new self();
  }

}
