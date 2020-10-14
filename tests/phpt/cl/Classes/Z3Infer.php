<?php

namespace Classes;

class Z3Infer
{
  public $a = 0;

  public function __construct() { }

  /**
   * @param int $x
   * @param int $y
   */
  public function thisHasInfer($x, $y) {
    echo $x, ' ', $y, "\n";
  }
}
