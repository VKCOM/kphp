<?php

namespace Ast\BinaryExpr;

use Ast\BinaryExpr;
use Ast\Expr;

class BinaryPlus extends BinaryExpr {
  /**
   * @param Expr $x
   * @param Expr $y
   */
  public function __construct($x, $y) {
    parent::__construct($x, $y);
  }
}
